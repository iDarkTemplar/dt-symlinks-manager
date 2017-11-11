/*
 * Copyright (C) 2017 i.Dark_Templar <darktemplar@dark-templar-archives.net>
 *
 * This file is part of DT Symlinks Manager.
 *
 * DT Symlinks Manager is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * DT Symlinks Manager is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with DT Symlinks Manager.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <string>
#include <vector>
#include <set>
#include <map>
#include <functional>
#include <sstream>
#include <fstream>

#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>

#include "input-reader.hpp"

#if USE_BOOST

#include <boost/optional.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/replace.hpp>

template <typename T>
using optional = boost::optional<T>;

static inline void to_lower(std::string &value)
{
	boost::to_lower(value);
}

static inline void trim(std::string &value)
{
	boost::trim(value);
}

static inline void replace_all(std::string &input, const char *search, const char *replace)
{
	boost::replace_all(input, search, replace);
}

#else /* USE_BOOST */

#include <algorithm>
#include <cctype>
#include <experimental/optional>

template <typename T>
using optional = std::experimental::optional<T>;

static inline void to_lower(std::string &value)
{
	std::transform(value.begin(), value.end(), value.begin(), ::tolower);
}

// trim from start (in place)
static inline void ltrim(std::string &value)
{
	value.erase(
		value.begin(),
		std::find_if(value.begin(), value.end(), [](int ch)
		{
			return !std::isspace(ch);
		}));
}

// trim from end (in place)
static inline void rtrim(std::string &value)
{
	value.erase(
		std::find_if(value.rbegin(), value.rend(), [](int ch)
		{
			return !std::isspace(ch);
		}).base(),
		value.end());
}

// trim from both ends (in place)
static inline void trim(std::string &value)
{
	ltrim(value);
	rtrim(value);
}

static inline void replace_all(std::string &input, const char *search, const char *replace)
{
	size_t pos = 0;
	size_t replaced_len = strlen(search);
	size_t replacing_len = strlen(replace);

	while ((pos = input.find(search, pos)) != std::string::npos)
	{
		input.replace(pos, replaced_len, replace);
		pos += replacing_len;
	} while (pos != std::string::npos);
}

#endif /* USE_BOOST */

enum class file_type
{
	not_existent,
	regular,
	directory,
	symlink_good,
	symlink_bad,
	symlink_nonexistent,
	unknown
};

struct entry_state
{
	bool file_is_present;
	file_type symlink_state;

	entry_state()
		: file_is_present(false),
		symlink_state(file_type::not_existent)
	{
	}
};

typedef std::map<std::string, entry_state> entries_state_map_t;

std::map<std::string, std::string> read_config(const std::string &config_file_name)
{
	std::map<std::string, std::string> result;

	std::ifstream infile(config_file_name);

	if (!infile.is_open())
	{
		std::stringstream err;
		err << "Failed to open config file " << config_file_name;
		throw std::runtime_error(err.str());
	}

	std::string line;
	size_t line_number = 0;

	while (std::getline(infile, line))
	{
		++line_number;
		trim(line);

		if (line.empty() || (line[0] == '#'))
		{
			continue;
		}

		std::string::size_type index = line.find('=');
		if ((index == std::string::npos) || (index == line.size() - 1))
		{
			std::stringstream err;
			err << "Invalid config line " << line_number << ": " << line;
			throw std::runtime_error(err.str());
		}

		std::string dir_source, dir_destination;
		dir_source      = line.substr(0, index);
		dir_destination = line.substr(index + 1);

		if ((dir_source.find('=') != std::string::npos) || (dir_destination.find('=') != std::string::npos))
		{
			std::stringstream err;
			err << "Invalid config line " << line_number << ": " << line;
			throw std::runtime_error(err.str());
		}

		trim(dir_source);
		trim(dir_destination);

		if (result.find(dir_destination) != result.end())
		{
			std::stringstream err;
			err << "Duplicate destination directory at config line " << line_number << ": " << line;
			throw std::runtime_error(err.str());
		}

		result[dir_destination] = dir_source;
	}

	return result;
}

void list_files_in_directory(const std::string &location, entries_state_map_t &entries_map)
{
	struct stat buffer;

	if (stat(location.c_str(), &buffer) != -1)
	{
		if (S_ISDIR(buffer.st_mode))
		{
			DIR *dirp;

			if ((dirp = opendir(location.c_str())) != NULL)
			{
				try
				{
					struct dirent *dp;

					for (;;)
					{
						dp = readdir(dirp);

						if (dp == NULL)
						{
							break;
						}

						if ((strcmp(dp->d_name,".") != 0) && (strcmp(dp->d_name,"..") != 0) && (dp->d_type == DT_REG))
						{
							entries_map[dp->d_name].file_is_present = true;
						}
					}
				}
				catch (...)
				{
					closedir(dirp);
					throw;
				}

				closedir(dirp);
			}
		}
	}
}

void string_replace_recursive(std::string &input, const char *search, const char *replace)
{
	size_t cur_size = input.size();
	size_t last_size = cur_size;

	for (;;)
	{
		replace_all(input, search, replace);
		cur_size = input.size();
		if (cur_size >= last_size)
		{
			break;
		}

		last_size = cur_size;
	}
}

std::string compose_filename(const std::string &directory_of_file, const std::string &filename)
{
	std::stringstream composed_filename_str;
	composed_filename_str << directory_of_file << '/' << filename;
	std::string composed_filename = composed_filename_str.str();

	string_replace_recursive(composed_filename, "//", "/");

	return composed_filename;
}

void list_files_in_directory_with_state(const std::string &location, const std::string &symlinks_destination, entries_state_map_t &entries_map)
{
	struct stat buffer;

	if (stat(location.c_str(), &buffer) != -1)
	{
		if (S_ISDIR(buffer.st_mode))
		{
			DIR *dirp;

			if ((dirp = opendir(location.c_str())) != NULL)
			{
				try
				{
					struct dirent *dp;

					for (;;)
					{
						dp = readdir(dirp);

						if (dp == NULL)
						{
							break;
						}

						if ((strcmp(dp->d_name,".") != 0) && (strcmp(dp->d_name,"..") != 0))
						{
							if (dp->d_type == DT_REG)
							{
								entries_map[dp->d_name].symlink_state = file_type::regular;
							}
							else if (dp->d_type == DT_DIR)
							{
								entries_map[dp->d_name].symlink_state = file_type::directory;
							}
							else if (dp->d_type == DT_LNK)
							{
								char link_buffer[PATH_MAX + 1];

								std::string filename = compose_filename(location, dp->d_name);

								ssize_t result = readlink(filename.c_str(), link_buffer, PATH_MAX);
								if (result >= 0)
								{
									std::string result_symlink = std::string(link_buffer, link_buffer + result);
									string_replace_recursive(result_symlink, "//", "/");

									std::string expected_symlink = compose_filename(symlinks_destination, dp->d_name);

									// if result does not exist, then it's nonexistent
									// if result does not match expected, it's bad
									// otherwise it's ok

									if (expected_symlink == result_symlink)
									{
										entries_map[dp->d_name].symlink_state = file_type::symlink_good;
									}
									else
									{
										if ((stat(filename.c_str(), &buffer) != -1) && (S_ISREG(buffer.st_mode)))
										{
											entries_map[dp->d_name].symlink_state = file_type::symlink_bad;
										}
										else
										{
											entries_map[dp->d_name].symlink_state = file_type::symlink_nonexistent;
										}
									}
								}
								else
								{
									entries_map[dp->d_name].symlink_state = file_type::unknown;
								}
							}
							else
							{
								entries_map[dp->d_name].symlink_state = file_type::unknown;
							}
						}
					}
				}
				catch (...)
				{
					closedir(dirp);
					throw;
				}

				closedir(dirp);
			}
		}
	}
}

void remove_file(const std::string &file)
{
	if (unlink(file.c_str()) < 0)
	{
		fprintf(stderr, "Failed to remove file: %s\n", file.c_str());
	}
}

void create_symlink(const std::string &real_location, const std::string &symlink_location)
{
	if (symlink(real_location.c_str(), symlink_location.c_str()) < 0)
	{
		fprintf(stderr, "Failed to create symlink %s pointing to %s\n", symlink_location.c_str(), real_location.c_str());
	}
}

void process_directory(InputReader &input_reader, const std::string &files_directory, const std::string &symlinks_directory)
{
	// first: read all files in files_directory
	// second: read all symlinks in symlinks directory and their locations.
	//	if symlinks is broken (dead), autoremove it
	//	if it's a file, report it
	//	if it's a symlink to a wrong location, report it
	//	for everything else, allow toggling

	bool run = true;

	do
	{
		entries_state_map_t entries_map;

		list_files_in_directory(files_directory, entries_map);
		list_files_in_directory_with_state(symlinks_directory, files_directory, entries_map);

		{
			auto iter_end = entries_map.end();

			for (auto iter = entries_map.begin(); iter != iter_end; )
			{
				if ((iter->second.file_is_present) || (iter->second.symlink_state != file_type::symlink_good))
				{
					++iter;
				}
				else
				{
					std::string expected_symlink = compose_filename(symlinks_directory, iter->first);
					remove_file(expected_symlink);

					iter = entries_map.erase(iter);
				}
			}
		}

		{
			fprintf(stdout, "\nSelect symlink to toggle state for:\n\n");

			size_t index = 0;
			auto iter_end = entries_map.end();

			for (auto iter = entries_map.begin(); iter != iter_end; ++iter, ++index)
			{
				char state = ' ';

				switch (iter->second.symlink_state)
				{
				case file_type::not_existent:
					state = ' ';
					break;

				case file_type::symlink_good:
					state = '*';
					break;

				case file_type::regular:
				case file_type::directory:
				case file_type::symlink_bad:
				case file_type::symlink_nonexistent:
				case file_type::unknown:
				default:
					state = 'E';
					break;
				}

				std::string full_name = compose_filename(files_directory, iter->first);

				fprintf(stdout, "%zu) [%c] %s\n", index, state, full_name.c_str());
			}

			fprintf(stdout, "q) Use \"q\" or \"quit\" to exit current menu\n\n");
		}

		for (;;)
		{
			fprintf(stdout, "> ");
			fflush(stdout);

			std::string command = input_reader.readInput();

			trim(command);
			to_lower(command);

			if (command.empty())
			{
				// NOTE: ignore empty command
			}
			else if ((command == "q") || (command == "quit"))
			{
				run = false;
				break;
			}
			else
			{
				optional<ssize_t> index;

				try
				{
					index = std::stoull(command);
				}
				catch (...)
				{
					// NOTE: ignore exception;
				}

				if (index && (*index >= 0) && (*index < entries_map.size()))
				{
					size_t current_index = 0;
					auto iter = entries_map.begin();
					auto iter_end = entries_map.end();

					for ( ; (current_index != *index) && (iter != iter_end); ++iter, ++current_index)
					{
					}

					if (iter != iter_end)
					{
						switch (iter->second.symlink_state)
						{
						case file_type::not_existent:
							{
								std::string real_location = compose_filename(files_directory, iter->first);
								std::string symlink_location = compose_filename(symlinks_directory, iter->first);
								create_symlink(real_location, symlink_location);
							}
							break;

						case file_type::symlink_good:
							{
								std::string symlink_location = compose_filename(symlinks_directory, iter->first);
								remove_file(symlink_location);
							}
							break;

						case file_type::regular:
						case file_type::directory:
						case file_type::symlink_bad:
						case file_type::symlink_nonexistent:
						case file_type::unknown:
						default:
							{
								std::string symlink_location = compose_filename(symlinks_directory, iter->first);
								fprintf(stderr, "Failed to toggle state of symlink %s: it's in invalid state, please check manually\n", symlink_location.c_str());
							}
							break;
						}
						break;
					}
				}
				else
				{
					fprintf(stderr, "Failed to recognize command: %s\n", command.c_str());
				}
			}
		}
	} while (run);
}

void print_help(const char *name)
{
	fprintf(stderr,
	       "USAGE: %s [options]\n"
	       "Options:\n"
	       "\t[-h] --help - shows this info\n"
	       "\t[-c config_file] --config config_file - use different config file instead of default one\n",
	       name);
}

int main(int argc, char **argv)
{
	try
	{
		bool help = false;
		bool run = true;
		std::string config_name = CONFIG_DIR "/dt-symlinks-manager.conf";

		std::map<std::string, std::string> directories_map;

		for (int i = 1; i < argc; ++i)
		{
			if ((strcmp(argv[i],"--help") == 0) || (strcmp(argv[i], "-h") == 0))
			{
				help = true;
			}
			else if ((strcmp(argv[i],"--config") == 0) || (strcmp(argv[i], "-c") == 0))
			{
				if (i < argc - 1)
				{
					++i;
					config_name = argv[i];
				}
				else
				{
					help = true;
				}
			}
			else
			{
				help = true;
			}
		}

		if (help)
		{
			print_help(argv[0]);
			return 0;
		}

		directories_map = read_config(config_name);

		InputReader input_reader(64 * 1024);

		do
		{
			{
				fprintf(stdout, "Select directory to modify symlinks in:\n\n");

				size_t index = 0;
				auto iter_end = directories_map.end();

				for (auto iter = directories_map.begin(); iter != iter_end; ++iter, ++index)
				{
					fprintf(stdout, "%zu) %s\n", index, iter->first.c_str());
				}

				fprintf(stdout, "q) Use \"q\" or \"quit\" to exit application\n\n");
			}

			for (;;)
			{
				fprintf(stdout, "> ");
				fflush(stdout);

				std::string command = input_reader.readInput();

				trim(command);
				to_lower(command);

				if (command.empty())
				{
					// NOTE: ignore empty command
				}
				else if ((command == "q") || (command == "quit"))
				{
					run = false;
					break;
				}
				else
				{
					optional<ssize_t> index;

					try
					{
						index = std::stoull(command);
					}
					catch (...)
					{
						// NOTE: ignore exception;
					}

					if (index && (*index >= 0) && (*index < directories_map.size()))
					{
						size_t current_index = 0;
						auto iter = directories_map.begin();
						auto iter_end = directories_map.end();

						for ( ; (current_index != *index) && (iter != iter_end); ++iter, ++current_index)
						{
						}

						if (iter != iter_end)
						{
							process_directory(input_reader, iter->second, iter->first);
							fprintf(stdout, "\n");
							break;
						}
					}
					else
					{
						fprintf(stderr, "Failed to recognize command: %s\n", command.c_str());
					}
				}
			}
		} while (run);
	}
	catch (const std::exception &exc)
	{
		fprintf(stderr, "Caught std::exception: %s\n", exc.what());
		return -1;
	}
	catch (...)
	{
		fprintf(stderr, "Caught unknown exception\n");
		return -1;
	}

	return 0;
}

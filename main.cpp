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

#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/case_conv.hpp>

#include "input-reader.hpp"

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
		boost::trim(line);

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

		boost::trim(dir_source);
		boost::trim(dir_destination);

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
			fprintf(stdout, "Select directory to modify symlinks in:\n\n");

			size_t index = 0;
			auto iter_end = directories_map.end();

			for (auto iter = directories_map.begin(); iter != iter_end; ++iter, ++index)
			{
				fprintf(stdout, "%zu) %s\n", index, iter->first.c_str());
			}

			fprintf(stdout, "q) Use \"q\" or \"quit\" to exit application\n\n");

			for (;;)
			{
				fprintf(stdout, "> ");
				fflush(stdout);

				std::string command = input_reader.readInput();

				boost::trim(command);
				boost::to_lower(command);

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
					boost::optional<ssize_t> index;

					try
					{
						index = boost::lexical_cast<ssize_t>(command);
					}
					catch (const boost::bad_lexical_cast &)
					{
						// NOTE: ignore exception;
					}

					if (index)
					{
						// TODO: process command: check that number is actually in range
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

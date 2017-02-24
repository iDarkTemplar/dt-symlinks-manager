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

#include "input-reader.hpp"

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <stdexcept>

InputReader::InputReader(size_t data_max)
	: m_data_used(0),
	m_data_max(data_max)
{
	m_data.resize(m_data_max + 1);
}

InputReader::~InputReader()
{
}

std::string InputReader::readInput()
{
	for (;;)
	{
		if (m_data_used > 0)
		{
			char *newline = (char*) memchr(m_data.data(), '\n', m_data_used);
			if (newline != NULL)
			{
				std::string command(m_data.data(), newline);

				m_data_used -= newline - m_data.data() + 1;
				if (m_data_used > 0)
				{
					memmove(m_data.data(), newline + 1, m_data_used);
				}

				return command;
			}
			else
			{
				if (m_data_used == m_data_max)
				{
					throw std::runtime_error("Failed to read command from input: command is too long");
				}
			}
		}

		// read stdin until some data arrived or error occurs
		for (;;)
		{
			ssize_t read_size = read(STDIN_FILENO, &(m_data[m_data_used]), m_data_max - m_data_used);
			if (read_size > 0)
			{
				m_data_used += read_size;
				break;
			}
			else if (read_size < 0)
			{
				throw std::runtime_error("Failed to read command from input");
			}
		}
	}
}

void InputReader::reset()
{
	m_data_used = 0;
}

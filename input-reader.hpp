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

#ifndef DT_INPUT_READER_HPP
#define DT_INPUT_READER_HPP

#include <string>
#include <vector>

class InputReader
{
public:
	explicit InputReader(size_t data_max);
	~InputReader();

	std::string readInput();
	void reset();

private:
	std::vector<char> m_data;
	size_t m_data_used;
	const size_t m_data_max;
};

#endif /* DT_INPUT_READER_HPP */

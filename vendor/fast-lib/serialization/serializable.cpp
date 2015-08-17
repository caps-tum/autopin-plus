/*
 * This file is part of fast-lib.
 * Copyright (C) 2015 RWTH Aachen University - ACS
 *
 * This file is licensed under the GNU Lesser General Public License Version 3
 * Version 3, 29 June 2007. For details see 'LICENSE.md' in the root directory.
 */

#include "serializable.hpp"

namespace fast
{
	std::string Serializable::to_string() const
	{
		return "---\n" + YAML::Dump(emit()) + "\n---";
	}
	void Serializable::from_string(const std::string &str)
	{
		load(YAML::Load(str));
	}
}

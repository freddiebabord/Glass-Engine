#pragma once

namespace GlassEngine
{
	/// Split a sting using a deliminator
	/// @param s String to split
	/// @param delim character used to split string
	/// @param result A collection of strings that have been split at the deliminator
	template<typename Out>
	void Split(const std::string &s, char delim, Out result) {
		std::stringstream ss;
		ss.str(s);
		std::string item;
		while (std::getline(ss, item, delim)) {
			*(result++) = item;
		}
	}

	/// Split a sting using a deliminator
	/// @param s String to split
	/// @param delim character used to split string
	/// @return A collection of strings that have been split at the deliminator
	std::vector<std::string> Split(const std::string& s, char delim);
}

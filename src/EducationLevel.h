#pragma once

#include <cctype>
#include <string>

enum class EducationLevel
{
	Any,
	HighSchool,
	Bachelors,
	Masters,
	PhD,
	Other
};

inline std::string normalizeEducationText(const std::string& value)
{
	std::string normalized;
	normalized.reserve(value.size());

	for (unsigned char ch : value)
	{
		if (std::isalnum(ch))
		{
			normalized.push_back(static_cast<char>(std::tolower(ch)));
		}
		else if (std::isspace(ch))
		{
			normalized.push_back(' ');
		}
	}

	return normalized;
}

inline EducationLevel educationLevelFromString(const std::string& value)
{
	const std::string normalized = normalizeEducationText(value);

	if (normalized.empty() || normalized.find("any") != std::string::npos)
	{
		return EducationLevel::Any;
	}
	if (normalized.find("high school") != std::string::npos)
	{
		return EducationLevel::HighSchool;
	}
	if (normalized.find("bachelor") != std::string::npos)
	{
		return EducationLevel::Bachelors;
	}
	if (normalized.find("master") != std::string::npos)
	{
		return EducationLevel::Masters;
	}
	if (normalized.find("phd") != std::string::npos)
	{
		return EducationLevel::PhD;
	}

	return EducationLevel::Other;
}

inline std::string educationLevelToString(EducationLevel level)
{
	switch (level)
	{
	case EducationLevel::Any:
		return "Any";
	case EducationLevel::HighSchool:
		return "High School";
	case EducationLevel::Bachelors:
		return "Bachelor's";
	case EducationLevel::Masters:
		return "Master's";
	case EducationLevel::PhD:
		return "PhD";
	default:
		return "Other";
	}
}

inline int educationLevelRank(EducationLevel level)
{
	switch (level)
	{
	case EducationLevel::HighSchool:
		return 1;
	case EducationLevel::Bachelors:
		return 2;
	case EducationLevel::Masters:
		return 3;
	case EducationLevel::PhD:
		return 4;
	default:
		return 0;
	}
}

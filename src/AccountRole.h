#pragma once

#include <string>

enum class AccountRole
{
	Unknown,
	Admin,
	Employer,
	Candidate
};

inline AccountRole accountRoleFromString(const std::string& role)
{
	if (role == "admin")
	{
		return AccountRole::Admin;
	}
	if (role == "employer")
	{
		return AccountRole::Employer;
	}
	if (role == "candidate")
	{
		return AccountRole::Candidate;
	}

	return AccountRole::Unknown;
}

inline std::string accountRoleToString(AccountRole role)
{
	switch (role)
	{
	case AccountRole::Admin:
		return "admin";
	case AccountRole::Employer:
		return "employer";
	case AccountRole::Candidate:
		return "candidate";
	default:
		return "";
	}
}

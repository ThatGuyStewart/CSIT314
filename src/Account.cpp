#include "Account.h"

Account::Account(std::string email, std::string password)
	: m_email(std::move(email)), m_password(std::move(password))
{
}

std::unique_ptr<Account> Account::create(AccountRole role, std::string email, std::string password)
{
	if (role == AccountRole::Admin)
	{
		return std::make_unique<AdminAccount>(std::move(email), std::move(password));
	}

	if (role == AccountRole::Employer)
	{
		return std::make_unique<EmployerAccount>(std::move(email), std::move(password));
	}

	if (role == AccountRole::Candidate)
	{
		return std::make_unique<CandidateAccount>(std::move(email), std::move(password));
	}

	return {};
}

std::string Account::getEmail() const
{
	return m_email;
}

bool Account::validatePassword(const std::string& password) const
{
	return m_password == password;
}

AdminAccount::AdminAccount(std::string email, std::string password)
	: Account(std::move(email), std::move(password))
{
}

AccountRole AdminAccount::getRole() const
{
	return AccountRole::Admin;
}

bool AdminAccount::isAdmin() const
{
	return true;
}

UserAccount::UserAccount(std::string email, std::string password)
	: Account(std::move(email), std::move(password))
{
}

bool UserAccount::isAdmin() const
{
	return false;
}

CandidateAccount::CandidateAccount(std::string email, std::string password)
	: UserAccount(std::move(email), std::move(password)), m_profile(new CandidateProfile)
{
}

AccountRole CandidateAccount::getRole() const
{
	return AccountRole::Candidate;
}

CandidateProfile& CandidateAccount::getProfile()
{
	return *m_profile;
}

bool CandidateAccount::isAdmin() const
{
	return false;
}

EmployerAccount::EmployerAccount(std::string email, std::string password)
	: UserAccount(std::move(email), std::move(password)), m_profile(new EmployerProfile)
{
}

AccountRole EmployerAccount::getRole() const
{
	return AccountRole::Employer;
}

bool EmployerAccount::isAdmin() const
{
	return false;
}

EmployerProfile& EmployerAccount::getProfile()
{
	return *m_profile;
}

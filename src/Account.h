#pragma once
#include <string>
#include <memory>
#include "AccountRole.h"
#include "Profile.h"

class Account
{
protected:
	std::string m_email;
	std::string m_password;
	Account(std::string email, std::string password);

public:
	virtual ~Account() = default;
	static std::unique_ptr<Account> create(AccountRole role, std::string email, std::string password);
	std::string getEmail() const;
	bool validatePassword(const std::string& password) const;
	virtual AccountRole getRole() const = 0;
	virtual bool isAdmin() const = 0;
};

class AdminAccount : public Account
{
public:
	AdminAccount(std::string email, std::string password);
	AccountRole getRole() const override;
	bool isAdmin() const override;
};

class UserAccount : public Account
{
	protected:
	UserAccount(std::string email, std::string password);

	public:
	bool isAdmin() const override;
};

class CandidateAccount : public UserAccount
{
private:
	std::unique_ptr<CandidateProfile> m_profile;
public:
	CandidateAccount(std::string email, std::string password);
	AccountRole getRole() const override;
	bool isAdmin() const override;
	CandidateProfile& getProfile();
};

class EmployerAccount : public UserAccount
{
private:
	std::unique_ptr<EmployerProfile> m_profile;
public:
	EmployerAccount(std::string email, std::string password);
	AccountRole getRole() const override;
	bool isAdmin() const override;
	EmployerProfile& getProfile();
};


#pragma once
#include <string>
#include "EducationLevel.h"
#include "Structs.h"

class Profile
{
	public:
		virtual ~Profile() = default;
};

class CandidateProfile : public Profile
{
private:
	std::string m_contactInfo;
	EducationLevel m_education = EducationLevel::Any;
	std::string m_major;
	int m_yearsExperience = 0;
	std::string m_workExperience;
	std::string m_skills;
	std::string m_preferredWorkMode;
	std::string m_preferredLocation;
	std::string m_expectedSalary;
	std::string m_employmentType;
	std::string m_summary;
	std::string m_portfolioUrl;
public:
	CandidateProfile();
	explicit CandidateProfile(const S_CandidateProfile& profile);
	CandidateProfile(std::string contactInfo,
		EducationLevel education,
		std::string major,
		int yearsExperience,
		std::string workExperience,
		std::string skills,
		std::string preferredWorkMode,
		std::string preferredLocation,
		std::string expectedSalary,
		std::string employmentType,
		std::string summary,
		std::string portfolioUrl);
	S_CandidateProfile toStruct() const;
};

class EmployerProfile : public Profile
{
private:
	std::string m_companyName;
	std::string m_companyEmail;
	std::string m_companyPhone;
	std::string m_industry;
	std::string m_companySize;
	std::string m_companyLocation;
	std::string m_companyDescription;
	std::string m_websiteUrl;
public:
	EmployerProfile();
	explicit EmployerProfile(const S_CompanyProfile& profile);
	EmployerProfile(std::string companyName,
		std::string companyEmail,
		std::string companyPhone,
		std::string industry,
		std::string companySize,
		std::string companyLocation,
		std::string companyDescription,
		std::string websiteUrl);
	S_CompanyProfile toStruct() const;
};
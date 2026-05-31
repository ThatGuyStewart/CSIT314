#include "Profile.h"

CandidateProfile::CandidateProfile() : Profile()
{
}

CandidateProfile::CandidateProfile(const S_CandidateProfile& profile)
	: CandidateProfile(profile.contactInfo,
		educationLevelFromString(profile.education),
		profile.major,
		profile.yearsExperience,
		profile.workExperience,
		profile.skills,
		profile.preferredWorkMode,
		profile.preferredLocation,
		profile.expectedSalary,
		profile.employmentType,
		profile.summary,
		profile.portfolioUrl)
{
}

CandidateProfile::CandidateProfile(std::string contactInfo, 
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
	std::string portfolioUrl)
	: Profile(),
	m_contactInfo(std::move(contactInfo)),
	m_education(education),
	m_major(std::move(major)),
	m_yearsExperience(yearsExperience),
	m_workExperience(std::move(workExperience)),
	m_skills(std::move(skills)),
	m_preferredWorkMode(std::move(preferredWorkMode)),
	m_preferredLocation(std::move(preferredLocation)),
	m_expectedSalary(std::move(expectedSalary)),
	m_employmentType(std::move(employmentType)),
	m_summary(std::move(summary)),
	m_portfolioUrl(std::move(portfolioUrl))
{
}

S_CandidateProfile CandidateProfile::toStruct() const
{
	return {
		m_contactInfo,
		educationLevelToString(m_education),
		m_major,
		m_yearsExperience,
		m_workExperience,
		m_skills,
		m_preferredWorkMode,
		m_preferredLocation,
		m_expectedSalary,
		m_employmentType,
		m_summary,
		m_portfolioUrl
	};
}

EmployerProfile::EmployerProfile() : Profile()
{
}

EmployerProfile::EmployerProfile(const S_CompanyProfile& profile)
	: EmployerProfile(profile.companyName,
		profile.companyEmail,
		profile.companyPhone,
		profile.industry,
		profile.companySize,
		profile.companyLocation,
		profile.companyDescription,
		profile.websiteUrl)
{
}

EmployerProfile::EmployerProfile(std::string companyName,
	std::string companyEmail,
	std::string companyPhone,
	std::string industry,
	std::string companySize,
	std::string companyLocation,
	std::string companyDescription,
	std::string websiteUrl)
	: Profile(),
	m_companyName(std::move(companyName)),
	m_companyEmail(std::move(companyEmail)),
	m_companyPhone(std::move(companyPhone)),
	m_industry(std::move(industry)),
	m_companySize(std::move(companySize)),
	m_companyLocation(std::move(companyLocation)),
	m_companyDescription(std::move(companyDescription)),
	m_websiteUrl(std::move(websiteUrl))
{
}

S_CompanyProfile EmployerProfile::toStruct() const
{
	return {
		m_companyName,
		m_companyEmail,
		m_companyPhone,
		m_industry,
		m_companySize,
		m_companyLocation,
		m_companyDescription,
		m_websiteUrl
	};
}

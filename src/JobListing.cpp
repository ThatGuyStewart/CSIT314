#include "JobListing.h"

JobListing::JobListing() = default;

JobListing::JobListing(const S_JobListing& listing)
	: JobListing(listing.companyInfo,
		listing.jobTitle,
		listing.jobDescription,
		educationLevelFromString(listing.requiredEducation),
		listing.requiredSkills,
		listing.requiredExperience,
		listing.workMode,
		listing.jobLocation,
		listing.salaryMin,
		listing.salaryMax,
		listing.jobType,
		listing.careerLevel,
		listing.applicationDeadline,
		listing.status)
{
}

JobListing::JobListing(std::string companyInfo,
	std::string jobTitle,
	std::string jobDescription,
	EducationLevel requiredEducation,
	std::string requiredSkills,
	int requiredExperience,
	std::string workMode,
	std::string jobLocation,
	std::string salaryMin,
	std::string salaryMax,
	std::string jobType,
	std::string careerLevel,
	std::string applicationDeadline,
	std::string status)
	: m_companyInfo(std::move(companyInfo)),
	m_jobTitle(std::move(jobTitle)),
	m_jobDescription(std::move(jobDescription)),
	m_requiredEducation(requiredEducation),
	m_requiredSkills(std::move(requiredSkills)),
	m_requiredExperience(requiredExperience),
	m_workMode(std::move(workMode)),
	m_jobLocation(std::move(jobLocation)),
	m_salaryMin(std::move(salaryMin)),
	m_salaryMax(std::move(salaryMax)),
	m_jobType(std::move(jobType)),
	m_careerLevel(std::move(careerLevel)),
	m_applicationDeadline(std::move(applicationDeadline)),
	m_status(std::move(status))
{
}

S_JobListing JobListing::toStruct() const
{
	return {
		m_companyInfo,
		m_jobTitle,
		m_jobDescription,
		educationLevelToString(m_requiredEducation),
		m_requiredSkills,
		m_requiredExperience,
		m_workMode,
		m_jobLocation,
		m_salaryMin,
		m_salaryMax,
		m_jobType,
		m_careerLevel,
		m_applicationDeadline,
		m_status
	};
}
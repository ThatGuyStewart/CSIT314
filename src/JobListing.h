#pragma once

#include "EducationLevel.h"
#include "Structs.h"

class JobListing
{
private:
	std::string m_companyInfo;
	std::string m_jobTitle;
	std::string m_jobDescription;
	EducationLevel m_requiredEducation = EducationLevel::Any;
	std::string m_requiredSkills;
	int m_requiredExperience = 0;
	std::string m_workMode;
	std::string m_jobLocation;
	std::string m_salaryMin;
	std::string m_salaryMax;
	std::string m_jobType;
	std::string m_careerLevel;
	std::string m_applicationDeadline;
	std::string m_status;

public:
	JobListing();
	explicit JobListing(const S_JobListing& listing);
	JobListing(std::string companyInfo,
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
		std::string status);

	S_JobListing toStruct() const;
};
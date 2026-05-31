#pragma once
#include <string>
#include <vector>

struct S_JobCard
{
	long long id = 0;
	std::string title;
	std::string company;
	std::string companyDescription;
	std::string companyWebsiteUrl;
	std::string type;
	std::string careerLevel;
	std::string status;
	std::string deadline;
	std::string posted;
	std::string location;
	std::string workMode;
	std::string salaryRange;
	std::string requiredSkills;
	std::string requiredEducation;
	int requiredExperience = 0;
	std::string jobDescription;
	std::string salaryMin;
	std::string salaryMax;
	int matchScore = 0;
	std::vector<std::string> matchReasons;
};

struct S_EmployerCandidateRecommendation
{
	long long candidateId = 0;
	std::string fullName;
	std::string major;
	std::string skills;
	std::string preferredLocation;
	std::string preferredWorkMode;
	std::string experienceText;
	std::string summary;
	long long jobId = 0;
	std::string jobTitle;
	float rawScore = 0.0f;
	int matchScore = 0;
	std::vector<std::string> matchReasons;
	std::vector<S_JobCard> matchedJobs;
};

struct S_CandidateCard
{
	long long id = 0;
	std::string fullName;
	std::string contactInfo;
	std::string education;
	std::string major;
	int yearsExperience = 0;
	std::string workExperience;
	std::string skills;
	std::string preferredLocation;
	std::string preferredWorkMode;
	std::string expectedSalary;
	std::string employmentType;
	std::string summary;
	std::string portfolioUrl;
	std::string createdAt;
	std::string experienceText;
	long long matchedJobId = 0;
	std::string matchedJobTitle;
	int matchScore = 0;
	std::vector<std::string> matchReasons;
};

struct S_CandidateProfile
{
	std::string contactInfo;
	std::string education;
	std::string major;
	int yearsExperience = 0;
	std::string workExperience;
	std::string skills;
	std::string preferredWorkMode;
	std::string preferredLocation;
	std::string expectedSalary;
	std::string employmentType;
	std::string summary;
	std::string portfolioUrl;
};

struct S_CompanyProfile
{
	std::string companyName;
	std::string companyEmail;
	std::string companyPhone;
	std::string industry;
	std::string companySize;
	std::string companyLocation;
	std::string companyDescription;
	std::string websiteUrl;
};

struct S_JobListing
{
	std::string companyInfo;
	std::string jobTitle;
	std::string jobDescription;
	std::string requiredEducation;
	std::string requiredSkills;
	int requiredExperience = 0;
	std::string workMode;
	std::string jobLocation;
	std::string salaryMin;
	std::string salaryMax;
	std::string jobType;
	std::string careerLevel;
	std::string applicationDeadline;
	std::string status;
};

struct S_UserSummary
{
	long long id = 0;
	std::string fullName;
	std::string email;
	std::string role;
	std::string membershipStatus;
	std::string profileOrJobs;
	std::string createdAt;
};

struct S_CandidateDashboardData
{
	int recommendedCount = 0;
	int profileComplete = 0;
	int openPositions = 0;
	std::string membershipStatus;
	std::vector<S_JobCard> topRecommendations;
};

struct S_EmployerDashboardData
{
	int activeCount = 0;
	int totalPosts = 0;
	int totalCandidates = 0;
	std::string membershipStatus;
	std::vector<S_CandidateCard> topCandidates;
};

struct S_AdminDashboardData
{
	int totalUsers = 0;
	int totalCandidates = 0;
	int totalEmployers = 0;
	int premiumUsers = 0;
	int totalJobs = 0;
	int openJobs = 0;
	int totalProfiles = 0;
	int totalCompanies = 0;
	std::vector<S_UserSummary> recentUsers;
	std::vector<S_JobCard> recentJobs;
};
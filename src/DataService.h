#pragma once

#include "Database.h"
#include <chrono>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

class DataService
{
private:
	Database& m_database;

public:
	explicit DataService(Database& database);
	~DataService();

	bool isConnected() const;
	bool validateAccount(const std::string& email, const std::string& password);
	bool accountExists(const std::string& email);
	bool createAccount(const std::string& fullName, const std::string& email, const std::string& password, const std::string& role);
	bool isAdminAccount(const std::string& email);
	std::optional<std::string> getAccountRole(const std::string& email);
	bool updateMembershipStatus(const std::string& email, const std::string& membershipStatus);

	std::optional<S_CandidateProfile> getCandidateProfile(const std::string& email);
	bool saveCandidateProfile(const std::string& email, const S_CandidateProfile& input);
	bool createJobPosting(const std::string& email, const S_JobListing& input);
	std::vector<S_UserSummary> getAdminUsers(const std::string& search, const std::string& role);
	S_CandidateDashboardData getCandidateDashboard(const std::string& email);
	std::vector<S_JobCard> getCandidateJobs(const std::string& keyword, const std::string& location, const std::string& workMode, const std::string& jobType, const std::string& careerLevel, const std::string& salaryMin, const std::string& salaryMax);
	std::optional<S_JobCard> getCandidateJobDetails(const std::string& email, long long jobId);
	bool applyToCandidateJob(const std::string& email, long long jobId);
	std::vector<S_JobCard> getRecommendedJobs(const std::string& email);
	S_EmployerDashboardData getEmployerDashboard(const std::string& email);
	std::vector<S_CandidateCard> getEmployerCandidates(const std::string& keyword, const std::string& skills, const std::string& education, const std::string& yearsExperience, const std::string& preferredWorkMode, const std::string& preferredLocation);
	std::optional<S_CandidateCard> getEmployerCandidateDetails(const std::string& email, long long candidateId, long long jobId = 0);
	std::vector<S_EmployerCandidateRecommendation> getEmployerRecommendations(const std::string& email, long long jobId = 0, size_t maxResults = 0);
	std::optional<S_JobCard> getEmployerJobDetails(const std::string& email, long long jobId);
	bool updateEmployerJob(const std::string& email, long long jobId, const S_JobListing& input);
	std::vector<S_JobCard> getEmployerJobs(const std::string& email);
	S_AdminDashboardData getAdminDashboard();
	std::vector<S_JobCard> getAdminJobs(const std::string& search, const std::string& status);
	std::optional<S_CandidateCard> getAdminCandidateDetails(long long candidateId);
	std::optional<S_AdminEmployerDetail> getAdminEmployerDetails(long long employerId);
	std::optional<S_JobCard> getAdminJobDetails(long long jobId);
	bool saveCompanyProfile(const std::string& email, const S_CompanyProfile& input);
	std::optional<S_CompanyProfile> getCompanyProfile(const std::string& email);
	void stop();
};

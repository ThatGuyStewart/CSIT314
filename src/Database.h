#pragma once
#include "Structs.h"
#include <memory>
#include <mutex>
#include <optional>
#include <pqxx/pqxx>

class Database
{
private:
	std::string m_host;
	std::string m_port;
	std::string m_dbname;
	std::string m_user;
	std::string m_password;
	mutable std::recursive_mutex m_dbMutex;
	std::unique_ptr<pqxx::connection> m_connection;

	bool connect(std::string& host, std::string& port, std::string& dbname, std::string& user, std::string& password);
	std::string loadSqlFile(const std::string& path);
	bool createDatabaseIfMissing();
	bool createSchema();

public:
	Database(std::string host, std::string port, std::string dbname, std::string user, std::string password);
	bool isConnected() const;
	bool createAccount(const std::string& fullName, const std::string& email, const std::string& password, const std::string& role);
	bool validateAccount(const std::string& email, const std::string& password);
	bool accountExists(const std::string& email);
	bool isAdminAccount(const std::string& email);
	std::optional<std::string> getAccountRole(const std::string& email);
	std::optional<S_CandidateProfile> getCandidateProfile(const std::string& email);
	std::string getCandidateSkills(const std::string& email);
	bool saveCandidateProfile(const std::string& email, const S_CandidateProfile& input);
	bool createJobPosting(const std::string& email, const S_JobListing& input);
	bool updateMembershipStatus(const std::string& email, const std::string& membershipStatus);
	std::vector<S_UserSummary> getAdminUsers(const std::string& role);
	S_CandidateDashboardData getCandidateDashboard(const std::string& email);
	std::vector<S_JobCard> getCandidateJobs();
	std::vector<S_JobCard> getFilteredCandidateJobs(const std::string& location, const std::string& workMode, const std::string& jobType, const std::string& careerLevel, const std::string& salaryMin, const std::string& salaryMax);
	std::optional<S_JobCard> getCandidateJobDetails(long long jobId);
	bool createCandidateApplication(const std::string& email, long long jobId);
	bool hasCandidateAppliedToJob(const std::string& email, long long jobId);
	S_EmployerDashboardData getEmployerDashboard(const std::string& email);
	std::vector<S_CandidateCard> getEmployerCandidates();
	std::vector<S_CandidateCard> getFilteredEmployerCandidates(const std::string& education, const std::string& yearsExperience, const std::string& preferredWorkMode, const std::string& preferredLocation);
	std::optional<S_CandidateCard> getEmployerCandidateDetails(long long candidateId);
	std::vector<S_JobCard> getEmployerJobs(const std::string& email);
	S_AdminDashboardData getAdminDashboard();
	std::vector<S_JobCard> getAdminJobs(const std::string& status);
	std::optional<S_CandidateCard> getAdminCandidateDetails(long long candidateId);
	std::optional<S_AdminEmployerDetail> getAdminEmployerDetails(long long employerId);
	std::optional<S_JobCard> getAdminJobDetails(long long jobId);
	bool saveCompanyProfile(const std::string& email, const S_CompanyProfile& input);
	std::optional<S_CompanyProfile> getCompanyProfile(const std::string& email);
};
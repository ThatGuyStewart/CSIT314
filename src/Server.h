#pragma once
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "DataService.h"
#include <string>
#include <httplib.h>
#include <unordered_map>
#include <mutex>
#include <memory>

// Session management struct.
struct Sessions
{
	std::mutex sessionMutex;
	std::unordered_map<std::string, std::string> tokenToUsername;
	std::unordered_map<std::string, std::string> usernameToToken;
};

// Main server class, inherits from SSLServer to support HTTPS.
// Contains all route definitions and session management logic.
class Server : public httplib::SSLServer
{
private:
	DataService& m_service;
	const std::string m_address;
	const int m_port;
	Sessions m_sessions;

	std::string loadFile(const std::string& path);
	std::string createToken();
	std::string getTokenFromCookie(const httplib::Request& req);
	std::string getUsernameForToken(const std::string& token);
	std::string getCookieValue(const httplib::Request& req, const std::string& cookieName);

	bool validateAccount(const std::string& email, const std::string& password);
	void createRoutes();
	void createFileRoute(const std::string& route, const std::string& path, const std::string& type);
	void serveHtmlFile(const std::string& path, httplib::Response& res);
	bool tryGetAuthenticatedUser(const httplib::Request& req, std::string& user);
	bool tryRequireAdmin(const httplib::Request& req, httplib::Response& res, std::string& user);
	
	void handleLogout(const httplib::Request& req, httplib::Response& res);
	void handleApiLogin(const httplib::Request& req, httplib::Response& res);
	void handleApiRegister(const httplib::Request& req, httplib::Response& res);
	void handleApiUser(const httplib::Request& req, httplib::Response& res);
	void handleApiCandidateProfileGet(const httplib::Request& req, httplib::Response& res);
	void handleApiCandidateProfile(const httplib::Request& req, httplib::Response& res);
	void handleApiCandidateDashboard(const httplib::Request& req, httplib::Response& res);
	void handleApiCandidateJobs(const httplib::Request& req, httplib::Response& res);
	void handleApiCandidateJobDetails(const httplib::Request& req, httplib::Response& res);
	void handleApiCandidateApplications(const httplib::Request& req, httplib::Response& res);
	void handleApiCandidateApply(const httplib::Request& req, httplib::Response& res);
	void handleApiCandidateRecommendedJobs(const httplib::Request& req, httplib::Response& res);
	void handleApiCandidateMembership(const httplib::Request& req, httplib::Response& res);
	void handleApiEmployerCreateJob(const httplib::Request& req, httplib::Response& res);
	void handleApiEmployerDashboard(const httplib::Request& req, httplib::Response& res);
	void handleApiEmployerCandidates(const httplib::Request& req, httplib::Response& res);
	void handleApiEmployerCandidateDetails(const httplib::Request& req, httplib::Response& res);
	void handleApiEmployerApplicants(const httplib::Request& req, httplib::Response& res);
	void handleApiEmployerRecommendations(const httplib::Request& req, httplib::Response& res);
	void handleApiEmployerMembership(const httplib::Request& req, httplib::Response& res);
	void handleApiEmployerJobs(const httplib::Request& req, httplib::Response& res);
	void handleApiEmployerEditJobGet(const httplib::Request& req, httplib::Response& res);
	void handleApiEmployerEditJobPost(const httplib::Request& req, httplib::Response& res);
	void handleApiEmployerCompanyProfileGet(const httplib::Request& req, httplib::Response& res);
	void handleApiEmployerCompanyProfilePost(const httplib::Request& req, httplib::Response& res);
	void handleApiAdminUsers(const httplib::Request& req, httplib::Response& res);
	void handleApiAdminDashboard(const httplib::Request& req, httplib::Response& res);
	void handleApiAdminJobs(const httplib::Request& req, httplib::Response& res);
	void handleApiAdminCandidateDetails(const httplib::Request& req, httplib::Response& res);
	void handleApiAdminEmployerDetails(const httplib::Request& req, httplib::Response& res);
	void handleApiAdminJobDetails(const httplib::Request& req, httplib::Response& res);
	bool tryRequireRole(const httplib::Request& req, httplib::Response& res, const std::string& requiredRole, std::string& user);
	void createProtectedPageRoute(const std::string& route, const std::string& role, const std::string& filePath);

public:
	Server(DataService& service, const char* cert = "./cert/cert.crt", const char* key = "./cert/cert.key", std::string address = "127.0.0.1", int port = 8080);
	~Server();
	void start();
	void stop();
};


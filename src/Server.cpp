#include "Server.h"
#include <algorithm>
#include <random>
#include <chrono>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <nlohmann/json.hpp>
#include <fstream>
#include <ctime>


Server::Server(DataService& service, const char* cert, const char* key, std::string address, int port)
	: httplib::SSLServer(cert, key), m_service(service), m_address(address), m_port(port)
{
	createRoutes();
}

Server::~Server()
{
	stop();
	if (m_broadcasterThread.thread.joinable())
	{
		m_broadcasterThread.thread.join();
	}
}

std::string Server::loadFile(const std::string& path) 
{
	std::ifstream ifs(path, std::ios::binary);
	if (!ifs) 
	{
		std::cout << "File " << path << " failed to load." << std::endl;
		return {};
	};
	std::ostringstream ss;
	ss << ifs.rdbuf();
	return ss.str();
}

std::string Server::createToken() 
{
	static std::mt19937_64 rng((unsigned)std::chrono::system_clock::now().time_since_epoch().count());
	std::uniform_int_distribution<uint64_t> dist;
	std::ostringstream token;
	token << std::hex << dist(rng) << dist(rng);
	return token.str();
}

std::string Server::getCookieValue(const httplib::Request& req, const std::string& cookieName)
{
	const std::string cookie = req.get_header_value("Cookie");
	const std::string prefix = cookieName + "=";

	auto pos = cookie.find(prefix);
	if (pos == std::string::npos) return {};

	pos += prefix.size();
	auto end = cookie.find(';', pos);
	return cookie.substr(pos, end == std::string::npos ? std::string::npos : end - pos);
}

std::string Server::getTokenFromCookie(const httplib::Request& req)
{
	return getCookieValue(req, "session");
}

std::string Server::getUsernameForToken(const std::string& token)
{
	if (token.empty()) return {};

	std::lock_guard<std::mutex> lock(m_sessions.sessionMutex);
	auto it = m_sessions.tokenToUsername.find(token);
	if (it == m_sessions.tokenToUsername.end()) return {};

	return it->second;
}

bool Server::validateAccount(const std::string& email, const std::string& password)
{
	return m_service.validateAccount(email, password);
}

void Server::serveHtmlFile(const std::string& path, httplib::Response& res)
{
	const auto body = loadFile(path);
	if (body.empty())
	{
		res.status = 404;
		res.set_content("Not found", "text/plain");
		return;
	}

	res.set_content(body, "text/html");
}

bool Server::tryGetAuthenticatedUser(const httplib::Request& req, std::string& user)
{
	const std::string token = getTokenFromCookie(req);
	user = getUsernameForToken(token);
	return !user.empty();
}

bool Server::tryRequireAdmin(const httplib::Request& req, httplib::Response& res, std::string& user)
{
	if (!tryGetAuthenticatedUser(req, user))
	{
		res.status = 302;
		res.set_header("Location", "/login");
		return false;
	}

	if (m_service.getAccountRole(user).value_or("") != "admin")
	{
		res.status = 403;
		res.set_content("Forbidden", "text/plain");
		return false;
	}

	return true;
}

void Server::createRoutes()
{
	createFileRoute("/", "HTML/index.html", "text/html");
	createFileRoute("/login", "HTML/login.html", "text/html");
	createFileRoute("/register", "HTML/register.html", "text/html");
	createFileRoute("/assets/js/main.js", "HTML/assets/js/main.js", "application/javascript");
	createFileRoute("/assets/css/style.css", "HTML/assets/css/style.css", "text/css");

	Post("/api/login", [&](const httplib::Request& req, httplib::Response& res) { handleApiLogin(req, res); });
	Get("/api/user", [&](const httplib::Request& req, httplib::Response& res) { handleApiUser(req, res); });
	Post("/api/register", [&](const httplib::Request& req, httplib::Response& res) { handleApiRegister(req, res); });
	Get("/api/candidate/profile", [&](const httplib::Request& req, httplib::Response& res) { handleApiCandidateProfileGet(req, res); });
	Post("/api/candidate/profile", [&](const httplib::Request& req, httplib::Response& res) { handleApiCandidateProfile(req, res); });
	Get("/api/candidate/dashboard", [&](const httplib::Request& req, httplib::Response& res) { handleApiCandidateDashboard(req, res); });
	Get("/api/candidate/jobs", [&](const httplib::Request& req, httplib::Response& res) { handleApiCandidateJobs(req, res); });
	Get("/api/candidate/job", [&](const httplib::Request& req, httplib::Response& res) { handleApiCandidateJobDetails(req, res); });
	Post("/api/candidate/apply", [&](const httplib::Request& req, httplib::Response& res) { handleApiCandidateApply(req, res); });
	Get("/api/candidate/recommended-jobs", [&](const httplib::Request& req, httplib::Response& res) { handleApiCandidateRecommendedJobs(req, res); });
	Post("/api/candidate/membership", [&](const httplib::Request& req, httplib::Response& res) { handleApiCandidateMembership(req, res); });
	Post("/api/employer/jobs", [&](const httplib::Request& req, httplib::Response& res) { handleApiEmployerCreateJob(req, res); });
	Get("/api/employer/dashboard", [&](const httplib::Request& req, httplib::Response& res) { handleApiEmployerDashboard(req, res); });
	Get("/api/employer/candidates", [&](const httplib::Request& req, httplib::Response& res) { handleApiEmployerCandidates(req, res); });
	Get("/api/employer/candidate", [&](const httplib::Request& req, httplib::Response& res) { handleApiEmployerCandidateDetails(req, res); });
	Get("/api/employer/recommendations", [&](const httplib::Request& req, httplib::Response& res) { handleApiEmployerRecommendations(req, res); });
	Post("/api/employer/membership", [&](const httplib::Request& req, httplib::Response& res) { handleApiEmployerMembership(req, res); });
	Get("/api/employer/company-profile", [&](const httplib::Request& req, httplib::Response& res) { handleApiEmployerCompanyProfileGet(req, res); });
	Get("/api/employer/company_profile", [&](const httplib::Request& req, httplib::Response& res) { handleApiEmployerCompanyProfileGet(req, res); });
	Post("/api/employer/company-profile", [&](const httplib::Request& req, httplib::Response& res) { handleApiEmployerCompanyProfilePost(req, res); });
	Post("/api/employer/company_profile", [&](const httplib::Request& req, httplib::Response& res) { handleApiEmployerCompanyProfilePost(req, res); });
	Get("/api/employer/jobs", [&](const httplib::Request& req, httplib::Response& res) { handleApiEmployerJobs(req, res); });
	Get("/api/admin/users", [&](const httplib::Request& req, httplib::Response& res) { handleApiAdminUsers(req, res); });
	Get("/api/admin/dashboard", [&](const httplib::Request& req, httplib::Response& res) { handleApiAdminDashboard(req, res); });
	Get("/api/admin/jobs", [&](const httplib::Request& req, httplib::Response& res) { handleApiAdminJobs(req, res); });
	Get("/api/admin/candidate", [&](const httplib::Request& req, httplib::Response& res) { handleApiAdminCandidateDetails(req, res); });
	Get("/api/admin/employer", [&](const httplib::Request& req, httplib::Response& res) { handleApiAdminEmployerDetails(req, res); });
	Get("/api/admin/job", [&](const httplib::Request& req, httplib::Response& res) { handleApiAdminJobDetails(req, res); });
	Get("/logout", [&](const httplib::Request& req, httplib::Response& res) { handleLogout(req, res); });

	createProtectedPageRoute("/candidate/dashboard", "candidate", "HTML/candidate/dashboard.html");
	createProtectedPageRoute("/candidate/profile", "candidate", "HTML/candidate/profile.html");
	createProtectedPageRoute("/candidate/jobs", "candidate", "HTML/candidate/jobs.html");
	createProtectedPageRoute("/candidate/job", "candidate", "HTML/candidate/job_details.html");
	createProtectedPageRoute("/candidate/recommended-jobs", "candidate", "HTML/candidate/recommended_jobs.html");
	createProtectedPageRoute("/candidate/membership", "candidate", "HTML/candidate/membership.html");

	createProtectedPageRoute("/employer/dashboard", "employer", "HTML/employer/dashboard.html");
	createProtectedPageRoute("/employer/company-profile", "employer", "HTML/employer/company_profile.html");
	createProtectedPageRoute("/employer/create-job", "employer", "HTML/employer/create_job.html");
	createProtectedPageRoute("/employer/manage-jobs", "employer", "HTML/employer/manage_jobs.html");
	createProtectedPageRoute("/employer/candidates", "employer", "HTML/employer/candidates.html");
	createProtectedPageRoute("/employer/candidate", "employer", "HTML/employer/candidate_details.html");
	createProtectedPageRoute("/employer/membership", "employer", "HTML/employer/membership.html");
	createProtectedPageRoute("/employer/company_profile", "employer", "HTML/employer/company_profile.html");
	createProtectedPageRoute("/employer/manage_jobs", "employer", "HTML/employer/manage_jobs.html");
	createProtectedPageRoute("/employer/browse_candidates", "employer", "HTML/employer/candidates.html");
	createProtectedPageRoute("/employer/browse-candidates", "employer", "HTML/employer/candidates.html");
	createProtectedPageRoute("/employer/recommended_candidates", "employer", "HTML/employer/recommended_candidates.html");
	createProtectedPageRoute("/employer/recommended-candidates", "employer", "HTML/employer/recommended_candidates.html");

	createProtectedPageRoute("/admin", "admin", "HTML/admin/dashboard.html");
	createProtectedPageRoute("/admin/dashboard", "admin", "HTML/admin/dashboard.html");
	createProtectedPageRoute("/admin/users", "admin", "HTML/admin/users.html");
	createProtectedPageRoute("/admin/jobs", "admin", "HTML/admin/jobs.html");
	createProtectedPageRoute("/admin/candidate", "admin", "HTML/admin/candidate_details.html");
	createProtectedPageRoute("/admin/employer", "admin", "HTML/admin/employer_details.html");
	createProtectedPageRoute("/admin/job", "admin", "HTML/admin/job_details.html");

	createWebsocketRoute();
}


void Server::createFileRoute(const std::string& route, const std::string& path, const std::string& type) 
{
	Get(route.c_str(), [this, path, type](const httplib::Request& /*req*/, httplib::Response& res) 
		{
		auto body = loadFile(path);
		if (body.empty()) 
		{
			res.status = 404;
			res.set_content("Not found", "text/plain");
			return;
		}
		res.set_content(body, type);
		});
}

void Server::handleApiLogin(const httplib::Request& req, httplib::Response& res)
{
	try
	{
		auto j = nlohmann::json::parse(req.body);
		std::string user = j.value("username", "");
		std::string pass = j.value("password", "");

		if (validateAccount(user, pass))
		{
			std::string token = createToken();
			{
				std::lock_guard<std::mutex> lock(m_sessions.sessionMutex);
				auto oldToken = m_sessions.usernameToToken.find(user);
				if (oldToken != m_sessions.usernameToToken.end())
				{
					m_sessions.tokenToUsername.erase(oldToken->second);
				}
				m_sessions.tokenToUsername[token] = user;
				m_sessions.usernameToToken[user] = token;
			}

			const std::string role = m_service.getAccountRole(user).value_or("");
			if (role != "admin" && role != "employer" && role != "candidate")
			{
				res.status = 403;
				res.set_content(R"({"error":"invalid account role"})", "application/json");
				return;
			}

			std::string redirectTo = "/candidate/dashboard";
			if (role == "admin")
			{
				redirectTo = "/admin";
			}
			else if (role == "employer")
			{
				redirectTo = "/employer/dashboard";
			}

			res.set_header("Set-Cookie", std::string("session=") + token + "; Path=/; HttpOnly; SameSite=Strict");

			nlohmann::json reply =
			{
				{"status", "ok"},
				{"user", user},
				{"role", role},
				{"redirectTo", redirectTo}
			};

			res.set_content(reply.dump(), "application/json");
			std::cout << " User " << user << " logged in with token " << token << std::endl;
			return;
		}

		res.status = 401;
		res.set_content(R"({"error":"Incorrect email or password."})", "application/json");
	}
	catch (const nlohmann::json::parse_error&)
	{
		res.status = 400;
		res.set_content(R"({"error":"invalid json"})", "application/json");
	}
}

void Server::handleApiEmployerRecommendations(const httplib::Request& req, httplib::Response& res)
{
	std::string user;
	if (!tryRequireRole(req, res, "employer", user)) return;

	long long jobId = 0;
	if (req.has_param("job_id"))
	{
		try
		{
			jobId = std::stoll(req.get_param_value("job_id"));
		}
		catch (const std::exception&)
		{
			res.status = 400;
			res.set_content(R"({"error":"invalid job id"})", "application/json");
			return;
		}
	}

	const auto recommendationsData = m_service.getEmployerRecommendations(user, jobId);

	nlohmann::json recommendations = nlohmann::json::array();
	for (const auto& item : recommendationsData)
	{
		nlohmann::json matchedJobs = nlohmann::json::array();
		for (const auto& matchedJob : item.matchedJobs)
		{
			matchedJobs.push_back({
				{"id", matchedJob.id},
				{"title", matchedJob.title},
				{"matchScore", matchedJob.matchScore}
				});
		}

		recommendations.push_back({
			{"candidateId", item.candidateId},
			{"fullName", item.fullName},
			{"major", item.major},
			{"skills", item.skills},
			{"preferredLocation", item.preferredLocation},
			{"preferredWorkMode", item.preferredWorkMode},
			{"experienceText", item.experienceText},
			{"summary", item.summary},
			{"jobId", item.jobId},
			{"jobTitle", item.jobTitle},
			{"matchScore", item.matchScore},
			{"matchReasons", item.matchReasons},
			{"matchedJobs", matchedJobs}
			});
	}

	res.set_content(nlohmann::json({ {"status", "ok"}, {"recommendations", recommendations} }).dump(), "application/json");
}

void Server::handleApiEmployerMembership(const httplib::Request& req, httplib::Response& res)
{
	std::string user;
	if (!tryRequireRole(req, res, "employer", user)) return;

	try
	{
		const auto body = nlohmann::json::parse(req.body);
		const std::string membershipStatus = body.value("membership_status", "");
		if (!m_service.updateMembershipStatus(user, membershipStatus))
		{
			res.status = 400;
			res.set_content(R"({"error":"failed to update membership status"})", "application/json");
			return;
		}

		res.set_content(nlohmann::json({ {"status", "ok"}, {"membershipStatus", membershipStatus} }).dump(), "application/json");
	}
	catch (const nlohmann::json::parse_error&)
	{
		res.status = 400;
		res.set_content(R"({"error":"invalid json"})", "application/json");
	}
}

void Server::handleApiCandidateProfileGet(const httplib::Request& req, httplib::Response& res)
{
	std::string user;
	if (!tryRequireRole(req, res, "candidate", user))
	{
		return;
	}

	const auto profile = m_service.getCandidateProfile(user);

	nlohmann::json reply =
	{
		{"status", "ok"},
		{"profile", nullptr}
	};

	if (profile.has_value())
	{
		reply["profile"] =
		{
			{"contact_info", profile->contactInfo},
			{"education", profile->education},
			{"major", profile->major},
			{"years_experience", profile->yearsExperience},
			{"work_experience", profile->workExperience},
			{"skills", profile->skills},
			{"preferred_work_mode", profile->preferredWorkMode},
			{"preferred_location", profile->preferredLocation},
			{"expected_salary", profile->expectedSalary},
			{"employment_type", profile->employmentType},
			{"summary", profile->summary},
			{"portfolio_url", profile->portfolioUrl}
		};
	}

	res.set_content(reply.dump(), "application/json");
}

void Server::handleApiRegister(const httplib::Request& req, httplib::Response& res)
{
	try
	{
		auto j = nlohmann::json::parse(req.body);

		const std::string fullName = j.value("full_name", "");
		const std::string email = j.value("email", "");
		const std::string password = j.value("password", "");
		const std::string role = j.value("role", "candidate");

		if (fullName.empty() || email.empty() || password.empty())
		{
			res.status = 400;
			res.set_content(R"({"error":"missing required fields"})", "application/json");
			return;
		}

		if (role != "candidate" && role != "employer")
		{
			res.status = 400;
			res.set_content(R"({"error":"invalid role"})", "application/json");
			return;
		}

		if (m_service.accountExists(email))
		{
			res.status = 409;
			res.set_content(R"({"error":"account already exists"})", "application/json");
			return;
		}

		if (!m_service.createAccount(fullName, email, password, role))
		{
			res.status = 500;
			res.set_content(R"({"error":"failed to create account"})", "application/json");
			return;
		}

		nlohmann::json reply =
		{
			{"status", "ok"},
			{"full_name", fullName},
			{"email", email},
			{"role", role}
		};

		res.set_content(reply.dump(), "application/json");
	}
	catch (const nlohmann::json::parse_error&)
	{
		res.status = 400;
		res.set_content(R"({"error":"invalid json"})", "application/json");
	}
}

void Server::handleApiUser(const httplib::Request& req, httplib::Response& res) 
{
	std::string token = getTokenFromCookie(req);
	if (!token.empty()) 
	{
		std::string user = getUsernameForToken(token);
		if (!user.empty()) 
		{
			nlohmann::json reply = { {"user", user} };
			res.set_content(reply.dump(), "application/json");
			return;
		}
	}
	res.status = 401;
	res.set_content(R"({"error":"unauthenticated"})", "application/json");
}

void Server::handleApiCandidateProfile(const httplib::Request& req, httplib::Response& res)
{
	std::string user;
	if (!tryRequireRole(req, res, "candidate", user))
	{
		return;
	}

	try
	{
		auto j = nlohmann::json::parse(req.body);

		S_CandidateProfile input;
		input.contactInfo = j.value("contact_info", "");
		input.education = j.value("education", "Bachelor's");
		input.major = j.value("major", "");
		input.yearsExperience = j.value("years_experience", 0);
		input.workExperience = j.value("work_experience", "");
		input.skills = j.value("skills", "");
		input.preferredWorkMode = j.value("preferred_work_mode", "Hybrid");
		input.preferredLocation = j.value("preferred_location", "");
		input.expectedSalary = j.value("expected_salary", "");
		input.employmentType = j.value("employment_type", "Full-time");
		input.summary = j.value("summary", "");
		input.portfolioUrl = j.value("portfolio_url", "");

		if (!m_service.saveCandidateProfile(user, input))
		{
			res.status = 500;
			res.set_content(R"({"error":"failed to save profile"})", "application/json");
			return;
		}

		res.set_content(R"({"status":"ok"})", "application/json");
	}
	catch (const nlohmann::json::parse_error&)
	{
		res.status = 400;
		res.set_content(R"({"error":"invalid json"})", "application/json");
	}
}

void Server::handleApiCandidateDashboard(const httplib::Request& req, httplib::Response& res)
{
	std::string user;
	if (!tryRequireRole(req, res, "candidate", user)) return;

	const auto data = m_service.getCandidateDashboard(user);

	nlohmann::json jobs = nlohmann::json::array();
	for (const auto& item : data.topRecommendations)
	{
		jobs.push_back({
			{"id", item.id},
			{"title", item.title},
			{"company", item.company},
			{"type", item.type},
			{"status", item.status},
			{"deadline", item.deadline},
			{"posted", item.posted},
			{"location", item.location},
			{"workMode", item.workMode},
			{"salaryRange", item.salaryRange},
			{"matchScore", item.matchScore},
			{"matchReasons", item.matchReasons}
			});
	}

	nlohmann::json reply = {
		{"status", "ok"},
		{"recommendedCount", data.recommendedCount},
		{"profileComplete", data.profileComplete},
		{"jobCount", data.openPositions},
		{"membershipStatus", data.membershipStatus},
		{"topRecommendations", jobs}
	};

	res.set_content(reply.dump(), "application/json");
}

void Server::handleApiCandidateJobs(const httplib::Request& req, httplib::Response& res)
{
	std::string user;
	if (!tryRequireRole(req, res, "candidate", user)) return;

	const std::string keyword = req.has_param("keyword") ? req.get_param_value("keyword") : "";
	const std::string location = req.has_param("location") ? req.get_param_value("location") : "";
	const std::string workMode = req.has_param("work_mode") ? req.get_param_value("work_mode") : "";
	const std::string jobType = req.has_param("job_type") ? req.get_param_value("job_type") : "";
	const std::string careerLevel = req.has_param("career_level") ? req.get_param_value("career_level") : "";
	const std::string salaryMin = req.has_param("salary_min") ? req.get_param_value("salary_min") : "";
	const std::string salaryMax = req.has_param("salary_max") ? req.get_param_value("salary_max") : "";

	const auto jobsData = m_service.getCandidateJobs(keyword, location, workMode, jobType, careerLevel, salaryMin, salaryMax);
	const auto dashboardData = m_service.getCandidateDashboard(user);

	nlohmann::json jobs = nlohmann::json::array();
	for (const auto& item : jobsData)
	{
		jobs.push_back({
			{"id", item.id},
			{"title", item.title},
			{"company", item.company},
			{"type", item.type},
			{"status", item.status},
			{"deadline", item.deadline},
			{"posted", item.posted},
			{"location", item.location},
			{"workMode", item.workMode},
			{"salaryRange", item.salaryRange},
			{"matchScore", item.matchScore},
			{"matchReasons", item.matchReasons}
			});
	}

	res.set_content(nlohmann::json({ {"status", "ok"}, {"membershipStatus", dashboardData.membershipStatus}, {"jobs", jobs} }).dump(), "application/json");
}

void Server::handleApiCandidateMembership(const httplib::Request& req, httplib::Response& res)
{
	std::string user;
	if (!tryRequireRole(req, res, "candidate", user)) return;

	try
	{
		const auto body = nlohmann::json::parse(req.body);
		const std::string membershipStatus = body.value("membership_status", "");
		if (!m_service.updateMembershipStatus(user, membershipStatus))
		{
			res.status = 400;
			res.set_content(R"({"error":"failed to update membership status"})", "application/json");
			return;
		}

		res.set_content(nlohmann::json({ {"status", "ok"}, {"membershipStatus", membershipStatus} }).dump(), "application/json");
	}
	catch (const nlohmann::json::parse_error&)
	{
		res.status = 400;
		res.set_content(R"({"error":"invalid json"})", "application/json");
	}
}

void Server::handleApiCandidateJobDetails(const httplib::Request& req, httplib::Response& res)
{
	std::string user;
	if (!tryRequireRole(req, res, "candidate", user)) return;

	if (!req.has_param("id"))
	{
		res.status = 400;
		res.set_content(R"({"error":"missing job id"})", "application/json");
		return;
	}

	try
	{
		const long long jobId = std::stoll(req.get_param_value("id"));
		const auto job = m_service.getCandidateJobDetails(user, jobId);
		if (!job.has_value())
		{
			res.status = 404;
			res.set_content(R"({"error":"job not found"})", "application/json");
			return;
		}

		nlohmann::json payload = {
			{"id", job->id},
			{"title", job->title},
			{"company", job->company},
			{"companyDescription", job->companyDescription},
			{"companyWebsiteUrl", job->companyWebsiteUrl},
			{"type", job->type},
			{"careerLevel", job->careerLevel},
			{"status", job->status},
			{"deadline", job->deadline},
			{"posted", job->posted},
			{"location", job->location},
			{"workMode", job->workMode},
			{"salaryRange", job->salaryRange},
			{"requiredSkills", job->requiredSkills},
			{"requiredEducation", job->requiredEducation},
			{"requiredExperience", job->requiredExperience},
			{"jobDescription", job->jobDescription},
			{"isApplied", job->isApplied},
			{"matchScore", job->matchScore},
			{"matchReasons", job->matchReasons}
		};

		res.set_content(nlohmann::json({ {"status", "ok"}, {"job", payload} }).dump(), "application/json");
	}
	catch (const std::exception&)
	{
		res.status = 400;
		res.set_content(R"({"error":"invalid job id"})", "application/json");
	}
}

void Server::handleApiCandidateApply(const httplib::Request& req, httplib::Response& res)
{
	std::string user;
	if (!tryRequireRole(req, res, "candidate", user)) return;

	try
	{
		const auto body = nlohmann::json::parse(req.body);
		const long long jobId = body.value("jobId", 0LL);
		if (jobId <= 0)
		{
			res.status = 400;
			res.set_content(R"({"error":"missing job id"})", "application/json");
			return;
		}

		if (!m_service.applyToCandidateJob(user, jobId))
		{
			res.status = 400;
			res.set_content(R"({"error":"failed to apply for job"})", "application/json");
			return;
		}

		res.set_content(R"({"status":"ok","isApplied":true})", "application/json");
	}
	catch (const nlohmann::json::parse_error&)
	{
		res.status = 400;
		res.set_content(R"({"error":"invalid json"})", "application/json");
	}
}

void Server::handleApiCandidateRecommendedJobs(const httplib::Request& req, httplib::Response& res)
{
	std::string user;
	if (!tryRequireRole(req, res, "candidate", user)) return;

	auto jobsData = m_service.getRecommendedJobs(user);
	const auto dashboardData = m_service.getCandidateDashboard(user);
	if (dashboardData.membershipStatus == "free" && jobsData.size() > 10)
	{
		jobsData.resize(10);
	}

	nlohmann::json jobs = nlohmann::json::array();
	for (const auto& item : jobsData)
	{
		jobs.push_back({
			{"id", item.id},
			{"title", item.title},
			{"company", item.company},
			{"type", item.type},
			{"status", item.status},
			{"deadline", item.deadline},
			{"posted", item.posted},
			{"location", item.location},
			{"workMode", item.workMode},
			{"salaryRange", item.salaryRange},
			{"matchScore", item.matchScore},
			{"matchReasons", item.matchReasons}
			});
	}

	res.set_content(nlohmann::json({ {"status", "ok"}, {"membershipStatus", dashboardData.membershipStatus}, {"jobs", jobs} }).dump(), "application/json");
}

void Server::handleApiEmployerCreateJob(const httplib::Request& req, httplib::Response& res)
{
	std::string user;
	if (!tryRequireRole(req, res, "employer", user))
	{
		return;
	}

	try
	{
		auto j = nlohmann::json::parse(req.body);

		S_JobListing input;
		input.companyInfo = j.value("company_info", "");
		input.jobTitle = j.value("job_title", "");
		input.jobDescription = j.value("job_description", "");
		input.requiredEducation = j.value("required_education", "Any");
		input.requiredSkills = j.value("required_skills", "");
		input.requiredExperience = j.value("required_experience", 0);
		input.workMode = j.value("work_mode", "On-site");
		input.jobLocation = j.value("job_location", "");
		input.salaryMin = j.value("salary_min", "");
		input.salaryMax = j.value("salary_max", "");
		input.jobType = j.value("job_type", "Full-time");
		input.careerLevel = j.value("career_level", "Entry-level");
		input.applicationDeadline = j.value("application_deadline", "");
		input.status = j.value("status", "Open");

		if (!m_service.createJobPosting(user, input))
		{
			res.status = 500;
			res.set_content(R"({"error":"failed to create job"})", "application/json");
			return;
		}

		res.set_content(R"({"status":"ok"})", "application/json");
	}
	catch (const nlohmann::json::parse_error&)
	{
		res.status = 400;
		res.set_content(R"({"error":"invalid json"})", "application/json");
	}
}

void Server::handleApiEmployerDashboard(const httplib::Request& req, httplib::Response& res)
{
	std::string user;
	if (!tryRequireRole(req, res, "employer", user)) return;

	const auto data = m_service.getEmployerDashboard(user);

	nlohmann::json candidates = nlohmann::json::array();
	for (const auto& item : data.topCandidates)
	{
		candidates.push_back({
			{"id", item.id},
			{"fullName", item.fullName},
			{"major", item.major},
			{"skills", item.skills},
			{"preferredLocation", item.preferredLocation},
			{"preferredWorkMode", item.preferredWorkMode},
			{"experienceText", item.experienceText},
			{"summary", item.summary},
			{"matchScore", item.matchScore},
			{"matchedJobId", item.matchedJobId},
			{"matchedJobTitle", item.matchedJobTitle}
			});
	}

	nlohmann::json reply = {
		{"status", "ok"},
		{"activeCount", data.activeCount},
		{"totalPosts", data.totalPosts},
		{"totalCandidates", data.totalCandidates},
		{"membershipStatus", data.membershipStatus},
		{"topCandidates", candidates}
	};

	res.set_content(reply.dump(), "application/json");
}

void Server::handleApiEmployerCandidates(const httplib::Request& req, httplib::Response& res)
{
	std::string user;
	if (!tryRequireRole(req, res, "employer", user)) return;

	const std::string keyword = req.has_param("keyword") ? req.get_param_value("keyword") : "";
	const std::string skills = req.has_param("skills") ? req.get_param_value("skills") : "";
	const std::string education = req.has_param("education") ? req.get_param_value("education") : "";
	const std::string yearsExperience = req.has_param("years_experience") ? req.get_param_value("years_experience") : "";
	const std::string preferredWorkMode = req.has_param("preferred_work_mode") ? req.get_param_value("preferred_work_mode") : "";
	const std::string preferredLocation = req.has_param("preferred_location") ? req.get_param_value("preferred_location") : "";

	const auto candidatesData = m_service.getEmployerCandidates(keyword, skills, education, yearsExperience, preferredWorkMode, preferredLocation);

	nlohmann::json candidates = nlohmann::json::array();
	for (const auto& item : candidatesData)
	{
		candidates.push_back({
			{"id", item.id},
			{"fullName", item.fullName},
			{"major", item.major},
			{"skills", item.skills},
			{"preferredLocation", item.preferredLocation},
			{"preferredWorkMode", item.preferredWorkMode},
			{"experienceText", item.experienceText}
			});
	}

	res.set_content(nlohmann::json({ {"status", "ok"}, {"candidates", candidates} }).dump(), "application/json");
}

void Server::handleApiEmployerCandidateDetails(const httplib::Request& req, httplib::Response& res)
{
	std::string user;
	if (!tryRequireRole(req, res, "employer", user)) return;

	if (!req.has_param("id"))
	{
		res.status = 400;
		res.set_content(R"({"error":"missing candidate id"})", "application/json");
		return;
	}

	try
	{
		const long long candidateId = std::stoll(req.get_param_value("id"));
		long long jobId = 0;
		if (req.has_param("job_id"))
		{
			jobId = std::stoll(req.get_param_value("job_id"));
		}

		const auto candidate = m_service.getEmployerCandidateDetails(user, candidateId, jobId);
		if (!candidate.has_value())
		{
			res.status = 404;
			res.set_content(R"({"error":"candidate not found"})", "application/json");
			return;
		}

		nlohmann::json payload = {
			{"id", candidate->id},
			{"fullName", candidate->fullName},
			{"contactInfo", candidate->contactInfo},
			{"education", candidate->education},
			{"major", candidate->major},
			{"yearsExperience", candidate->yearsExperience},
			{"workExperience", candidate->workExperience},
			{"skills", candidate->skills},
			{"preferredLocation", candidate->preferredLocation},
			{"preferredWorkMode", candidate->preferredWorkMode},
			{"expectedSalary", candidate->expectedSalary},
			{"employmentType", candidate->employmentType},
			{"summary", candidate->summary},
			{"portfolioUrl", candidate->portfolioUrl},
			{"createdAt", candidate->createdAt},
			{"experienceText", candidate->experienceText},
			{"matchedJobId", candidate->matchedJobId},
			{"matchedJobTitle", candidate->matchedJobTitle},
			{"matchScore", candidate->matchScore},
			{"matchReasons", candidate->matchReasons}
		};

		res.set_content(nlohmann::json({ {"status", "ok"}, {"candidate", payload} }).dump(), "application/json");
	}
	catch (const std::exception&)
	{
		res.status = 400;
		res.set_content(R"({"error":"invalid candidate id"})", "application/json");
	}
}

void Server::handleApiEmployerJobs(const httplib::Request& req, httplib::Response& res)
{
	std::string user;
	if (!tryRequireRole(req, res, "employer", user)) return;

	const auto jobsData = m_service.getEmployerJobs(user);

	nlohmann::json jobs = nlohmann::json::array();
	for (const auto& item : jobsData)
	{
		jobs.push_back({
			{"id", item.id},
			{"title", item.title},
			{"company", item.company},
			{"type", item.type},
			{"status", item.status},
			{"deadline", item.deadline},
			{"posted", item.posted},
			{"location", item.location},
			{"workMode", item.workMode},
			{"salaryRange", item.salaryRange}
			});
	}

	res.set_content(nlohmann::json({ {"status", "ok"}, {"jobs", jobs} }).dump(), "application/json");
}

void Server::handleApiAdminUsers(const httplib::Request& req, httplib::Response& res)
{
	std::string user;
	if (!tryRequireAdmin(req, res, user))
	{
		return;
	}

	const std::string search = req.has_param("search") ? req.get_param_value("search") : "";
	const std::string role = req.has_param("role") ? req.get_param_value("role") : "";

	const auto users = m_service.getAdminUsers(search, role);

	nlohmann::json items = nlohmann::json::array();
	for (const auto& item : users)
	{
		items.push_back(
			{
				{"id", item.id},
				{"full_name", item.fullName},
				{"email", item.email},
				{"role", item.role},
				{"membership_status", item.membershipStatus},
				{"profile_or_jobs", item.profileOrJobs},
				{"created_at", item.createdAt}
			});
	}

	nlohmann::json reply =
	{
		{"status", "ok"},
		{"users", items}
	};

	res.set_content(reply.dump(), "application/json");
}

void Server::handleApiAdminDashboard(const httplib::Request& req, httplib::Response& res)
{
	std::string user;
	if (!tryRequireAdmin(req, res, user)) return;

	const auto data = m_service.getAdminDashboard();

	nlohmann::json recentUsers = nlohmann::json::array();
	for (const auto& item : data.recentUsers)
	{
		recentUsers.push_back({
			{"id", item.id},
			{"full_name", item.fullName},
			{"email", item.email},
			{"role", item.role},
			{"membership_status", item.membershipStatus},
			{"profile_or_jobs", item.profileOrJobs},
			{"created_at", item.createdAt}
			});
	}

	nlohmann::json recentJobs = nlohmann::json::array();
	for (const auto& item : data.recentJobs)
	{
		recentJobs.push_back({
			{"id", item.id},
			{"title", item.title},
			{"company", item.company},
			{"type", item.type},
			{"status", item.status},
			{"deadline", item.deadline},
			{"posted", item.posted}
			});
	}

	nlohmann::json reply = {
		{"status", "ok"},
		{"totalUsers", data.totalUsers},
		{"totalCandidates", data.totalCandidates},
		{"totalEmployers", data.totalEmployers},
		{"premiumUsers", data.premiumUsers},
		{"totalJobs", data.totalJobs},
		{"openJobs", data.openJobs},
		{"totalProfiles", data.totalProfiles},
		{"totalCompanies", data.totalCompanies},
		{"recentUsers", recentUsers},
		{"recentJobs", recentJobs}
	};

	res.set_content(reply.dump(), "application/json");
}

void Server::handleApiAdminJobs(const httplib::Request& req, httplib::Response& res)
{
	std::string user;
	if (!tryRequireAdmin(req, res, user)) return;

	const std::string search = req.has_param("search") ? req.get_param_value("search") : "";
	const std::string status = req.has_param("status") ? req.get_param_value("status") : "";

	const auto jobsData = m_service.getAdminJobs(search, status);

	nlohmann::json jobs = nlohmann::json::array();
	for (const auto& item : jobsData)
	{
		jobs.push_back({
			{"id", item.id},
			{"title", item.title},
			{"company", item.company},
			{"type", item.type},
			{"status", item.status},
			{"deadline", item.deadline},
			{"posted", item.posted}
			});
	}

	res.set_content(nlohmann::json({ {"status", "ok"}, {"jobs", jobs} }).dump(), "application/json");
}

void Server::handleApiAdminCandidateDetails(const httplib::Request& req, httplib::Response& res)
{
	std::string user;
	if (!tryRequireAdmin(req, res, user)) return;
	if (!req.has_param("id"))
	{
		res.status = 400;
		res.set_content(R"({"error":"missing candidate id"})", "application/json");
		return;
	}

	try
	{
		const long long candidateId = std::stoll(req.get_param_value("id"));
		const auto candidate = m_service.getAdminCandidateDetails(candidateId);
		if (!candidate.has_value())
		{
			res.status = 404;
			res.set_content(R"({"error":"candidate not found"})", "application/json");
			return;
		}

		nlohmann::json payload = {
			{"id", candidate->id},
			{"fullName", candidate->fullName},
			{"contactInfo", candidate->contactInfo},
			{"education", candidate->education},
			{"major", candidate->major},
			{"yearsExperience", candidate->yearsExperience},
			{"workExperience", candidate->workExperience},
			{"skills", candidate->skills},
			{"preferredLocation", candidate->preferredLocation},
			{"preferredWorkMode", candidate->preferredWorkMode},
			{"expectedSalary", candidate->expectedSalary},
			{"employmentType", candidate->employmentType},
			{"summary", candidate->summary},
			{"portfolioUrl", candidate->portfolioUrl},
			{"createdAt", candidate->createdAt},
			{"experienceText", candidate->experienceText}
		};

		res.set_content(nlohmann::json({ {"status", "ok"}, {"candidate", payload} }).dump(), "application/json");
	}
	catch (const std::exception&)
	{
		res.status = 400;
		res.set_content(R"({"error":"invalid candidate id"})", "application/json");
	}
}

void Server::handleApiAdminEmployerDetails(const httplib::Request& req, httplib::Response& res)
{
	std::string user;
	if (!tryRequireAdmin(req, res, user)) return;
	if (!req.has_param("id"))
	{
		res.status = 400;
		res.set_content(R"({"error":"missing employer id"})", "application/json");
		return;
	}

	try
	{
		const long long employerId = std::stoll(req.get_param_value("id"));
		const auto employer = m_service.getAdminEmployerDetails(employerId);
		if (!employer.has_value())
		{
			res.status = 404;
			res.set_content(R"({"error":"employer not found"})", "application/json");
			return;
		}

		nlohmann::json payload = {
			{"id", employer->id},
			{"fullName", employer->fullName},
			{"email", employer->email},
			{"membershipStatus", employer->membershipStatus},
			{"createdAt", employer->createdAt},
			{"companyName", employer->companyName},
			{"companyEmail", employer->companyEmail},
			{"companyPhone", employer->companyPhone},
			{"industry", employer->industry},
			{"companySize", employer->companySize},
			{"companyLocation", employer->companyLocation},
			{"companyDescription", employer->companyDescription},
			{"websiteUrl", employer->websiteUrl},
			{"totalJobs", employer->totalJobs}
		};

		res.set_content(nlohmann::json({ {"status", "ok"}, {"employer", payload} }).dump(), "application/json");
	}
	catch (const std::exception&)
	{
		res.status = 400;
		res.set_content(R"({"error":"invalid employer id"})", "application/json");
	}
}

void Server::handleApiAdminJobDetails(const httplib::Request& req, httplib::Response& res)
{
	std::string user;
	if (!tryRequireAdmin(req, res, user)) return;
	if (!req.has_param("id"))
	{
		res.status = 400;
		res.set_content(R"({"error":"missing job id"})", "application/json");
		return;
	}

	try
	{
		const long long jobId = std::stoll(req.get_param_value("id"));
		const auto job = m_service.getAdminJobDetails(jobId);
		if (!job.has_value())
		{
			res.status = 404;
			res.set_content(R"({"error":"job not found"})", "application/json");
			return;
		}

		nlohmann::json payload = {
			{"id", job->id},
			{"title", job->title},
			{"company", job->company},
			{"companyDescription", job->companyDescription},
			{"companyWebsiteUrl", job->companyWebsiteUrl},
			{"type", job->type},
			{"careerLevel", job->careerLevel},
			{"status", job->status},
			{"deadline", job->deadline},
			{"posted", job->posted},
			{"location", job->location},
			{"workMode", job->workMode},
			{"salaryRange", job->salaryRange},
			{"requiredSkills", job->requiredSkills},
			{"requiredEducation", job->requiredEducation},
			{"requiredExperience", job->requiredExperience},
			{"jobDescription", job->jobDescription}
		};

		res.set_content(nlohmann::json({ {"status", "ok"}, {"job", payload} }).dump(), "application/json");
	}
	catch (const std::exception&)
	{
		res.status = 400;
		res.set_content(R"({"error":"invalid job id"})", "application/json");
	}
}

void Server::handleLogout(const httplib::Request& req, httplib::Response& res)
{
	const std::string token = getTokenFromCookie(req);

	if (!token.empty())
	{
		std::lock_guard<std::mutex> lock(m_sessions.sessionMutex);

		auto tokenIt = m_sessions.tokenToUsername.find(token);
		if (tokenIt != m_sessions.tokenToUsername.end())
		{
			const std::string user = tokenIt->second;
			m_sessions.tokenToUsername.erase(tokenIt);

			auto userIt = m_sessions.usernameToToken.find(user);
			if (userIt != m_sessions.usernameToToken.end() && userIt->second == token)
			{
				m_sessions.usernameToToken.erase(userIt);
			}
		}
	}

	res.status = 302;
	res.set_header("Set-Cookie", "session=; Path=/; Max-Age=0; HttpOnly; SameSite=Strict");
	res.set_header("Location", "/login");
}

void Server::handleApiEmployerCompanyProfileGet(const httplib::Request& req, httplib::Response& res)
{
	std::string user;
	if (!tryRequireRole(req, res, "employer", user)) return;

	const auto profile = m_service.getCompanyProfile(user);

	nlohmann::json reply =
	{
		{"status", "ok"},
		{"profile", nullptr}
	};

	if (profile.has_value())
	{
		reply["profile"] =
		{
			{"company_name", profile->companyName},
			{"company_email", profile->companyEmail},
			{"company_phone", profile->companyPhone},
			{"industry", profile->industry},
			{"company_size", profile->companySize},
			{"company_location", profile->companyLocation},
			{"company_description", profile->companyDescription},
			{"website_url", profile->websiteUrl}
		};
	}

	res.set_content(reply.dump(), "application/json");
}

void Server::handleApiEmployerCompanyProfilePost(const httplib::Request& req, httplib::Response& res)
{
	std::string user;
	if (!tryRequireRole(req, res, "employer", user))
	{
		return;
	}

	try
	{
		auto j = nlohmann::json::parse(req.body);

		S_CompanyProfile input;
		input.companyName = j.value("company_name", "");
		input.companyEmail = j.value("company_email", "");
		input.companyPhone = j.value("company_phone", "");
		input.industry = j.value("industry", "Technology");
		input.companySize = j.value("company_size", "11-50");
		input.companyLocation = j.value("company_location", "");
		input.companyDescription = j.value("company_description", "");
		input.websiteUrl = j.value("website_url", "");

		if (input.companyName.empty())
		{
			res.status = 400;
			res.set_content(R"({"error":"company name is required"})", "application/json");
			return;
		}

		if (input.industry.empty()) input.industry = "Technology";
		if (input.companySize.empty()) input.companySize = "11-50";

		if (!m_service.saveCompanyProfile(user, input))
		{
			res.status = 500;
			res.set_content(R"({"error":"failed to save company profile"})", "application/json");
			return;
		}

		res.set_content(R"({"status":"ok"})", "application/json");
	}
	catch (const nlohmann::json::parse_error&)
	{
		res.status = 400;
		res.set_content(R"({"error":"invalid json"})", "application/json");
	}
}

void Server::createWebsocketRoute() 
{
	WebSocket("/ws", [&](const httplib::Request& req, httplib::ws::WebSocket& ws)
		{
			std::string token = getTokenFromCookie(req);
			if (!token.empty())
			{
				std::lock_guard<std::mutex> lock(m_sessions.sessionMutex);
				if (m_sessions.tokenToUsername.find(token) == m_sessions.tokenToUsername.end()) token.clear();
			}

			{
				std::lock_guard<std::mutex> lock(m_sessions.websocketMutex);
				m_sessions.websockets.insert(&ws);
				m_sessions.websocketToToken[&ws] = token;
			}

			if (!token.empty())
			{


				const std::string user = getUsernameForToken(token);
				if (!user.empty())
				{

				}
			}

			std::string msg;
			while (ws.is_open())
			{
				auto res = ws.read(msg);
				if (res == httplib::ws::ReadResult::Fail) break;
				if (res == httplib::ws::ReadResult::Text)
				{
					handleWebsocketMessage(ws, msg);
				}
			}

			cleanupOnClose(ws);
		});
}

void Server::handleWebsocketMessage(httplib::ws::WebSocket& ws, const std::string& msg)
{
	try
	{
		auto parsed = nlohmann::json::parse(msg);
		auto type = parsed.value("type", std::string());

		if (type == "login")
		{
			std::string user = parsed.value("username", std::string());
			std::string pass = parsed.value("password", std::string());

			if (validateAccount(user, pass))
			{
				std::string newToken = createToken();
				{
					std::lock_guard<std::mutex> lock(m_sessions.sessionMutex);
					auto oldToken = m_sessions.usernameToToken.find(user);
					if (oldToken != m_sessions.usernameToToken.end())
					{
						m_sessions.tokenToUsername.erase(oldToken->second);
					}

					m_sessions.tokenToUsername[newToken] = user;
					m_sessions.usernameToToken[user] = newToken;
				}

				const std::string role = m_service.getAccountRole(user).value_or("");
				if (role != "admin" && role != "employer" && role != "candidate")
				{
					nlohmann::json reply =
					{
						{"type", "login"},
						{"status", "error"},
						{"error", "invalid account role"}
					};

					ws.send(reply.dump());
					return;
				}

				{
					std::lock_guard<std::mutex> lock2(m_sessions.websocketMutex);
					m_sessions.websocketToToken[&ws] = newToken;
				}

				nlohmann::json reply =
				{
					{"type", "login"},
					{"status", "ok"},
					{"token", newToken},
					{"user", user},
					{"role", role}
				};

				ws.send(reply.dump());

			}
			else
			{
				nlohmann::json reply =
				{
					{"type", "login"},
					{"status", "error"},
					{"error", "invalid credentials"}
				};

			ws.send(reply.dump());
			}
		}
	}
	catch (...)
	{
		std::cout << "Error handling websocket message. Ignoring." << std::endl;
	}
}

void Server::cleanupOnClose(httplib::ws::WebSocket& ws)
{
	std::string associatedToken;

	{
		std::lock_guard<std::mutex> lock(m_sessions.websocketMutex);
		m_sessions.websockets.erase(&ws);

		auto websocketIt = m_sessions.websocketToToken.find(&ws);
		if (websocketIt != m_sessions.websocketToToken.end())
		{
			associatedToken = websocketIt->second;
			m_sessions.websocketToToken.erase(websocketIt);
		}
	}

	if (!associatedToken.empty())
	{
		std::lock_guard<std::mutex> lock(m_sessions.sessionMutex);

		auto tokenIt = m_sessions.tokenToUsername.find(associatedToken);
		if (tokenIt != m_sessions.tokenToUsername.end())
		{
			const std::string user = tokenIt->second;
			m_sessions.tokenToUsername.erase(tokenIt);

			auto userIt = m_sessions.usernameToToken.find(user);
			if (userIt != m_sessions.usernameToToken.end() && userIt->second == associatedToken)
			{
				m_sessions.usernameToToken.erase(userIt);
			}
		}
	}
}

void Server::startWebsocketBroadcaster()
{
	if (m_broadcasterThread.thread.joinable()) return;

	m_broadcasterThread.thread = std::thread([this]()
	{
		while (m_broadcasterThread.running.load())
		{

			std::unique_lock<std::mutex> lock(m_broadcasterThread.mutex);
			m_broadcasterThread.cv.wait_for(
				lock,
				std::chrono::seconds(1),
				[this]() { return !m_broadcasterThread.running.load(); });
		}
	});
}

bool Server::tryRequireRole(const httplib::Request& req, httplib::Response& res, const std::string& requiredRole, std::string& user)
{
	if (!tryGetAuthenticatedUser(req, user))
	{
		res.status = 302;
		res.set_header("Location", "/login");
		return false;
	}

	if (m_service.getAccountRole(user).value_or("") != requiredRole)
	{
		res.status = 403;
		res.set_content("Forbidden", "text/plain");
		return false;
	}

	return true;
}

void Server::createProtectedPageRoute(const std::string& route, const std::string& role, const std::string& filePath)
{
	Get(route.c_str(), [this, role, filePath](const httplib::Request& req, httplib::Response& res)
		{
			std::string user;
			if (!tryRequireRole(req, res, role, user))
			{
				return;
			}

			serveHtmlFile(filePath, res);
		});
}

void Server::start() 
{
	if (m_broadcasterThread.running.exchange(true)) return;
	try 
	{
		std::cout << "Server starting at https://" << m_address << ":" << m_port << "\nPress Ctrl+C to stop and quit." << std::endl;
		startWebsocketBroadcaster();
		if (!listen(m_address, m_port)) 
		{
			m_broadcasterThread.running.store(false);
			m_broadcasterThread.cv.notify_all();
			if (m_broadcasterThread.thread.joinable())
			{
				m_broadcasterThread.thread.join();
			}
			throw std::runtime_error("Server failed to start.");
		}
	}
	catch (const std::exception& e) 
	{
		std::cout << "Runtime error. " << e.what() << " Server not running." << std::endl;
		m_broadcasterThread.running.store(false);
		m_broadcasterThread.cv.notify_all();
		if (m_broadcasterThread.thread.joinable())
		{
			m_broadcasterThread.thread.join();
		}
		throw;
	}
	catch (...) 
	{
		std::cout << "Unexpected error. Server not running." << std::endl;
		m_broadcasterThread.running.store(false);
		m_broadcasterThread.cv.notify_all();
		if (m_broadcasterThread.thread.joinable())
		{
			m_broadcasterThread.thread.join();
		}
		throw;
	}

	m_broadcasterThread.running.store(false);
	m_broadcasterThread.cv.notify_all();
	if (m_broadcasterThread.thread.joinable())
	{
		m_broadcasterThread.thread.join();
	}
}

void Server::stop() 
{
	if (!m_broadcasterThread.running.exchange(false)) return;

	m_broadcasterThread.cv.notify_all();

	std::vector<httplib::ws::WebSocket*> websocketsToClose;
	{
		std::lock_guard<std::mutex> lock(m_sessions.websocketMutex);
		websocketsToClose.reserve(m_sessions.websockets.size());

		for (httplib::ws::WebSocket* ws : m_sessions.websockets)
		{
			if (ws != nullptr)
			{
				websocketsToClose.push_back(ws);
			}
		}
	}

	for (httplib::ws::WebSocket* ws : websocketsToClose)
	{
		try
		{
			ws->close(httplib::ws::CloseStatus::GoingAway, "Server shutting down");
		}
		catch (...)
		{
		}
	}

	httplib::SSLServer::stop();
}

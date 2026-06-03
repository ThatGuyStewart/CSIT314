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

// Generate a secure random token for session management, using the current time as a seed for randomness. The token is created by generating two random 64-bit integers and concatenating their hexadecimal representations to form a unique session identifier.
std::string Server::createToken() 
{
	static std::mt19937_64 rng((unsigned)std::chrono::system_clock::now().time_since_epoch().count());
	std::uniform_int_distribution<uint64_t> dist;
	std::ostringstream token;
	token << std::hex << dist(rng) << dist(rng);
	return token.str();
}

// Extract the value of a specific cookie from the "Cookie" header in the HTTP request. The function searches for the specified cookie name followed by an equals sign, and if found, it retrieves the value of that cookie until it reaches a semicolon or the end of the string. If the cookie is not found, an empty string is returned.
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

// Retrieve the session token from the cookies in the HTTP request by calling the getCookieValue function with the specific cookie name "session". This function is used to identify the user's session and is essential for managing user authentication and maintaining session state across requests.
std::string Server::getTokenFromCookie(const httplib::Request& req)
{
	return getCookieValue(req, "session");
}

// Look up the username associated with a given session token by accessing the tokenToUsername map in the Sessions structure. The function first checks if the provided token is empty, and if not, it locks the session mutex to ensure thread safety while accessing the shared data structure. It then searches for the token in the map, and if found, it returns the corresponding username; otherwise, it returns an empty string.
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

// Create a route that serves a specific file with the given content type. The function takes the route path, the file path, and the content type as parameters. When a request is made to the specified route, the server will call the serveHtmlFile function to load the file and set the appropriate content type in the response.
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

// Check if the user is authenticated by attempting to retrieve the username associated with the session token from the request. The function first extracts the token from the cookies, then looks up the username for that token. If a valid username is found, it returns true and sets the user parameter to the username; otherwise, it returns false, indicating that the user is not authenticated.
bool Server::tryGetAuthenticatedUser(const httplib::Request& req, std::string& user)
{
	const std::string token = getTokenFromCookie(req);
	user = getUsernameForToken(token);
	return !user.empty();
}

// Check if the user is authenticated and has an admin role. The function first calls tryGetAuthenticatedUser to verify if the user is logged in, and if not, it redirects the response to the login page. If the user is authenticated, it then checks the user's role using the DataService's getAccountRole method. If the role is not "admin", it sets the response status to 403 Forbidden and returns false; otherwise, it returns true, allowing access to admin-only routes.
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
	Get("/api/candidate/applications", [&](const httplib::Request& req, httplib::Response& res) { handleApiCandidateApplications(req, res); });
	Post("/api/candidate/apply", [&](const httplib::Request& req, httplib::Response& res) { handleApiCandidateApply(req, res); });
	Get("/api/candidate/recommended-jobs", [&](const httplib::Request& req, httplib::Response& res) { handleApiCandidateRecommendedJobs(req, res); });
	Post("/api/candidate/membership", [&](const httplib::Request& req, httplib::Response& res) { handleApiCandidateMembership(req, res); });
	Post("/api/employer/jobs", [&](const httplib::Request& req, httplib::Response& res) { handleApiEmployerCreateJob(req, res); });
	Get("/api/employer/dashboard", [&](const httplib::Request& req, httplib::Response& res) { handleApiEmployerDashboard(req, res); });
	Get("/api/employer/candidates", [&](const httplib::Request& req, httplib::Response& res) { handleApiEmployerCandidates(req, res); });
	Get("/api/employer/candidate", [&](const httplib::Request& req, httplib::Response& res) { handleApiEmployerCandidateDetails(req, res); });
	Get("/api/employer/applicants", [&](const httplib::Request& req, httplib::Response& res) { handleApiEmployerApplicants(req, res); });
	Get("/api/employer/recommendations", [&](const httplib::Request& req, httplib::Response& res) { handleApiEmployerRecommendations(req, res); });
	Post("/api/employer/membership", [&](const httplib::Request& req, httplib::Response& res) { handleApiEmployerMembership(req, res); });
	Get("/api/employer/company-profile", [&](const httplib::Request& req, httplib::Response& res) { handleApiEmployerCompanyProfileGet(req, res); });
	Get("/api/employer/company_profile", [&](const httplib::Request& req, httplib::Response& res) { handleApiEmployerCompanyProfileGet(req, res); });
	Post("/api/employer/company-profile", [&](const httplib::Request& req, httplib::Response& res) { handleApiEmployerCompanyProfilePost(req, res); });
	Post("/api/employer/company_profile", [&](const httplib::Request& req, httplib::Response& res) { handleApiEmployerCompanyProfilePost(req, res); });
	Get("/api/employer/jobs", [&](const httplib::Request& req, httplib::Response& res) { handleApiEmployerJobs(req, res); });
	Get("/api/employer/edit-job", [&](const httplib::Request& req, httplib::Response& res) { handleApiEmployerEditJobGet(req, res); });
	Post("/api/employer/edit-job", [&](const httplib::Request& req, httplib::Response& res) { handleApiEmployerEditJobPost(req, res); });
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
	createProtectedPageRoute("/candidate/applications", "candidate", "HTML/candidate/applications.html");
	createProtectedPageRoute("/candidate/job", "candidate", "HTML/candidate/job_details.html");
	createProtectedPageRoute("/candidate/recommended-jobs", "candidate", "HTML/candidate/recommended_jobs.html");
	createProtectedPageRoute("/candidate/membership", "candidate", "HTML/candidate/membership.html");

	createProtectedPageRoute("/employer/dashboard", "employer", "HTML/employer/dashboard.html");
	createProtectedPageRoute("/employer/company-profile", "employer", "HTML/employer/company_profile.html");
	createProtectedPageRoute("/employer/create-job", "employer", "HTML/employer/create_job.html");
	createProtectedPageRoute("/employer/manage-jobs", "employer", "HTML/employer/manage_jobs.html");
	createProtectedPageRoute("/employer/edit-job", "employer", "HTML/employer/edit_job.html");
	createProtectedPageRoute("/employer/applicants", "employer", "HTML/employer/applicants.html");
	createProtectedPageRoute("/employer/candidates", "employer", "HTML/employer/candidates.html");
	createProtectedPageRoute("/employer/candidate", "employer", "HTML/employer/candidate_details.html");
	createProtectedPageRoute("/employer/candidate-details", "employer", "HTML/employer/candidate_details.html");
	createProtectedPageRoute("/employer/candidate_details", "employer", "HTML/employer/candidate_details.html");
	createProtectedPageRoute("/employer/membership", "employer", "HTML/employer/membership.html");
	createProtectedPageRoute("/employer/company_profile", "employer", "HTML/employer/company_profile.html");
	createProtectedPageRoute("/employer/edit_job", "employer", "HTML/employer/edit_job.html");
	createProtectedPageRoute("/employer/view-applicants", "employer", "HTML/employer/applicants.html");
	createProtectedPageRoute("/employer/view_applicants", "employer", "HTML/employer/applicants.html");
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
}

// Create a route that serves a specific file with the given content type when accessed. The function takes the route path, the file path, and the content type as parameters. When a GET request is made to the specified route, the server will load the file using the loadFile function and set the response content to the file's contents with the appropriate content type. If the file cannot be loaded, it will return a 404 Not Found response.
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

// Handle the API endpoint for user login. The function parses the JSON body of the request to extract the username and password, then validates the credentials using the validateAccount method. If the credentials are valid, it creates a new session token, updates the session mappings, and returns a JSON response containing the user's role and a redirect URL based on their role. If the credentials are invalid or if there is an error in parsing the JSON, it returns an appropriate error response.
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

// Handle the API endpoint for retrieving employer recommendations. The function first checks if the user is authenticated and has the "employer" role. It then optionally retrieves a job ID from the query parameters and calls the getEmployerRecommendations method of the DataService to fetch the recommendations data. The recommendations are formatted into a JSON response, which includes details about each recommended candidate and their matched jobs, and returned to the client. If the user is not authenticated or does not have the correct role, an appropriate error response is returned.
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

// Handle the API endpoint for updating employer membership status. The function first checks if the user is authenticated and has the "employer" role. It then parses the JSON body of the request to extract the new membership status and calls the updateMembershipStatus method of the DataService to update the status in the database. If the update is successful, it returns a JSON response with the new membership status; otherwise, it returns an error response indicating that the update failed. If there is an error in parsing the JSON, it returns a 400 Bad Request response.
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

// Handle the API endpoint for retrieving a candidate's profile. The function first checks if the user is authenticated and has the "candidate" role. It then calls the getCandidateProfile method of the DataService to fetch the candidate's profile data. The profile data is formatted into a JSON response, which includes various details about the candidate's background, skills, and preferences, and returned to the client. If the user is not authenticated or does not have the correct role, an appropriate error response is returned.
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

// Handle the API endpoint for user registration. The function parses the JSON body of the request to extract the user's full name, email, password, and role. It validates the input data and checks if an account with the provided email already exists. If the input is valid and the account does not exist, it creates a new account using the createAccount method of the DataService. Upon successful account creation, it returns a JSON response with the user's details; otherwise, it returns an appropriate error response based on the validation failure or account creation issue. If there is an error in parsing the JSON, it returns a 400 Bad Request response.
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

// Handle the API endpoint for retrieving the authenticated user's information. The function extracts the session token from the cookies in the request and looks up the associated username. If a valid username is found, it returns a JSON response containing the username; otherwise, it returns a 401 Unauthorized response indicating that the user is unauthenticated.
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

// Handle the API endpoint for updating a candidate's profile. The function first checks if the user is authenticated and has the "candidate" role. It then parses the JSON body of the request to extract various profile fields such as contact information, education, skills, and preferences. The extracted data is used to create an S_CandidateProfile object, which is then passed to the saveCandidateProfile method of the DataService to update the candidate's profile in the database. If the update is successful, it returns a JSON response indicating success; otherwise, it returns an error response. If there is an error in parsing the JSON, it returns a 400 Bad Request response.
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

// Handle the API endpoint for retrieving a candidate's dashboard data. The function first checks if the user is authenticated and has the "candidate" role. It then calls the getCandidateDashboard method of the DataService to fetch the dashboard data, which includes recommended jobs, profile completion status, and membership status. The data is formatted into a JSON response, which includes details about each recommended job and returned to the client. If the user is not authenticated or does not have the correct role, an appropriate error response is returned.
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

// Handle the API endpoint for retrieving jobs for a candidate based on search criteria. The function first checks if the user is authenticated and has the "candidate" role. It then retrieves various search parameters from the query string, such as keyword, location, work mode, job type, career level, and salary range. These parameters are passed to the getCandidateJobs method of the DataService to fetch matching job listings. The job data is formatted into a JSON response, which includes details about each job and returned to the client along with the candidate's membership status. If the user is not authenticated or does not have the correct role, an appropriate error response is returned.
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

// Handle the API endpoint for updating a candidate's membership status. The function first checks if the user is authenticated and has the "candidate" role. It then parses the JSON body of the request to extract the new membership status and calls the updateMembershipStatus method of the DataService to update the status in the database. If the update is successful, it returns a JSON response with the new membership status; otherwise, it returns an error response indicating that the update failed. If there is an error in parsing the JSON, it returns a 400 Bad Request response.
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

// Handle the API endpoint for retrieving detailed information about a specific job for a candidate. The function first checks if the user is authenticated and has the "candidate" role. It then retrieves the job ID from the query parameters and calls the getCandidateJobDetails method of the DataService to fetch detailed information about the specified job. The job details are formatted into a JSON response, which includes various attributes of the job and returned to the client. If the job is not found, it returns a 404 Not Found response; if the job ID is invalid, it returns a 400 Bad Request response. If the user is not authenticated or does not have the correct role, an appropriate error response is returned.
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

// Handle the API endpoint for retrieving a candidate's job applications. The function first checks if the user is authenticated and has the "candidate" role. It then calls the getCandidateApplications method of the DataService to fetch a list of jobs that the candidate has applied to. The job application data is formatted into a JSON response, which includes details about each job and returned to the client. If the user is not authenticated or does not have the correct role, an appropriate error response is returned.
void Server::handleApiCandidateApplications(const httplib::Request& req, httplib::Response& res)
{
	std::string user;
	if (!tryRequireRole(req, res, "candidate", user)) return;

	const auto jobsData = m_service.getCandidateApplications(user);

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
			{"isApplied", item.isApplied}
		});
	}

	res.set_content(nlohmann::json({ {"status", "ok"}, {"jobs", jobs} }).dump(), "application/json");
}

// Handle the API endpoint for applying to or removing an application from a job for a candidate. The function first checks if the user is authenticated and has the "candidate" role. It then parses the JSON body of the request to extract the job ID and a flag indicating whether to remove the application. Based on the flag, it either applies to the specified job or removes the existing application using the DataService. If the operation is successful, it returns a JSON response indicating the new application status; otherwise, it returns an error response. If there is an error in parsing the JSON, it returns a 400 Bad Request response.
void Server::handleApiCandidateApply(const httplib::Request& req, httplib::Response& res)
{
	std::string user;
	if (!tryRequireRole(req, res, "candidate", user)) return;

	try
	{
		const auto body = nlohmann::json::parse(req.body);
		const long long jobId = body.value("jobId", 0LL);
		const bool removeApplication = body.value("removeApplication", false);
		if (jobId <= 0)
		{
			res.status = 400;
			res.set_content(R"({"error":"missing job id"})", "application/json");
			return;
		}

		const bool ok = removeApplication
			? m_service.removeCandidateApplication(user, jobId)
			: m_service.applyToCandidateJob(user, jobId);

		if (!ok)
		{
			res.status = 400;
			res.set_content(removeApplication
				? R"({"error":"failed to remove application"})"
				: R"({"error":"failed to apply for job"})", "application/json");
			return;
		}

		res.set_content((removeApplication
			? R"({"status":"ok","isApplied":false})"
			: R"({"status":"ok","isApplied":true})"), "application/json");
	}
	catch (const nlohmann::json::parse_error&)
	{
		res.status = 400;
		res.set_content(R"({"error":"invalid json"})", "application/json");
	}
}

// Handle the API endpoint for retrieving recommended jobs for a candidate. The function first checks if the user is authenticated and has the "candidate" role. It then calls the getRecommendedJobs method of the DataService to fetch a list of recommended jobs for the candidate, as well as their dashboard data to determine their membership status. If the candidate has a free membership and there are more than 10 recommended jobs, the list is truncated to 10 items. The job data is formatted into a JSON response, which includes details about each recommended job and returned to the client along with the candidate's membership status. If the user is not authenticated or does not have the correct role, an appropriate error response is returned.
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

// Handle the API endpoint for creating a new job posting by an employer. The function first checks if the user is authenticated and has the "employer" role. It then parses the JSON body of the request to extract various job details such as company information, job title, description, requirements, location, salary range, and other attributes. The extracted data is used to create an S_JobListing object, which is then passed to the createJobPosting method of the DataService to save the new job posting in the database. If the creation is successful, it returns a JSON response indicating success; otherwise, it returns an error response. If there is an error in parsing the JSON, it returns a 400 Bad Request response.
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

// Handle the API endpoint for retrieving an employer's dashboard data. The function first checks if the user is authenticated and has the "employer" role. It then calls the getEmployerDashboard method of the DataService to fetch the dashboard data, which includes top candidate matches, active job count, total posts, total candidates, and membership status. The data is formatted into a JSON response, which includes details about each top candidate match and returned to the client. If the user is not authenticated or does not have the correct role, an appropriate error response is returned.
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

// Handle the API endpoint for retrieving candidates for an employer based on search criteria. The function first checks if the user is authenticated and has the "employer" role. It then retrieves various search parameters from the query string, such as keyword, skills, education, years of experience, preferred work mode, and preferred location. These parameters are passed to the getEmployerCandidates method of the DataService to fetch matching candidate profiles. The candidate data is formatted into a JSON response, which includes details about each candidate and returned to the client. If the user is not authenticated or does not have the correct role, an appropriate error response is returned.
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

// Handle the API endpoint for retrieving detailed information about a specific candidate for an employer. The function first checks if the user is authenticated and has the "employer" role. It then retrieves the candidate ID from the query parameters and calls the getEmployerCandidateDetails method of the DataService to fetch detailed information about the specified candidate. The candidate details are formatted into a JSON response, which includes various attributes of the candidate's profile, matched job information, and returned to the client. If the candidate is not found, it returns a 404 Not Found response; if the candidate ID is invalid, it returns a 400 Bad Request response. If the user is not authenticated or does not have the correct role, an appropriate error response is returned.
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

// Handle the API endpoint for retrieving applicants for an employer's job posting. The function first checks if the user is authenticated and has the "employer" role. It then retrieves the job ID from the query parameters and calls the getEmployerApplicants method of the DataService to fetch a list of applicants for the specified job. The applicant data is formatted into a JSON response, which includes details about each applicant and returned to the client along with the job information. If the job is not found, it returns a 404 Not Found response; if the job ID is invalid, it returns a 400 Bad Request response. If the user is not authenticated or does not have the correct role, an appropriate error response is returned.
void Server::handleApiEmployerApplicants(const httplib::Request& req, httplib::Response& res)
{
	std::string user;
	if (!tryRequireRole(req, res, "employer", user)) return;

	const auto jobsData = m_service.getEmployerApplicants(user);

	nlohmann::json jobs = nlohmann::json::array();
	for (const auto& job : jobsData)
	{
		nlohmann::json applicants = nlohmann::json::array();
		for (const auto& applicant : job.applicants)
		{
			applicants.push_back({
				{"id", applicant.id},
				{"fullName", applicant.fullName},
				{"major", applicant.major},
				{"skills", applicant.skills},
				{"preferredLocation", applicant.preferredLocation},
				{"preferredWorkMode", applicant.preferredWorkMode},
				{"experienceText", applicant.experienceText},
				{"summary", applicant.summary}
			});
		}

		jobs.push_back({
			{"jobId", job.jobId},
			{"jobTitle", job.jobTitle},
			{"status", job.status},
			{"location", job.location},
			{"workMode", job.workMode},
			{"applicantCount", job.applicantCount},
			{"applicants", applicants}
		});
	}

	res.set_content(nlohmann::json({ {"status", "ok"}, {"jobs", jobs} }).dump(), "application/json");
}

// Handle the API endpoint for retrieving applicants for an employer's job posting. The function first checks if the user is authenticated and has the "employer" role. It then retrieves the job ID from the query parameters and calls the getEmployerApplicants method of the DataService to fetch a list of applicants for the specified job. The applicant data is formatted into a JSON response, which includes details about each applicant and returned to the client along with the job information. If the job is not found, it returns a 404 Not Found response; if the job ID is invalid, it returns a 400 Bad Request response. If the user is not authenticated or does not have the correct role, an appropriate error response is returned.
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

// Handle the API endpoint for retrieving detailed information about a specific job for an employer to edit. The function first checks if the user is authenticated and has the "employer" role. It then retrieves the job ID from the query parameters and calls the getEmployerJobDetails method of the DataService to fetch detailed information about the specified job. The job details are formatted into a JSON response, which includes various attributes of the job and returned to the client. If the job is not found, it returns a 404 Not Found response; if the job ID is invalid, it returns a 400 Bad Request response. If the user is not authenticated or does not have the correct role, an appropriate error response is returned.
void Server::handleApiEmployerEditJobGet(const httplib::Request& req, httplib::Response& res)
{
	std::string user;
	if (!tryRequireRole(req, res, "employer", user)) return;

	if (!req.has_param("id"))
	{
		res.status = 400;
		res.set_content(R"({"error":"missing job id"})", "application/json");
		return;
	}

	try
	{
		const long long jobId = std::stoll(req.get_param_value("id"));
		const auto job = m_service.getEmployerJobDetails(user, jobId);
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
			{"salaryMin", job->salaryMin},
			{"salaryMax", job->salaryMax}
		};

		res.set_content(nlohmann::json({ {"status", "ok"}, {"job", payload} }).dump(), "application/json");
	}
	catch (const std::exception&)
	{
		res.status = 400;
		res.set_content(R"({"error":"invalid job id"})", "application/json");
	}
}

// Handle the API endpoint for updating a job posting by an employer. The function first checks if the user is authenticated and has the "employer" role. It then parses the JSON body of the request to extract various job details such as job title, description, requirements, location, salary range, and other attributes. The extracted data is used to create an S_JobListing object, which is then passed to the updateEmployerJob method of the DataService along with the job ID to update the existing job posting in the database. If the update is successful, it returns a JSON response indicating success; otherwise, it returns an error response. If there is an error in parsing the JSON, it returns a 400 Bad Request response.
void Server::handleApiEmployerEditJobPost(const httplib::Request& req, httplib::Response& res)
{
	std::string user;
	if (!tryRequireRole(req, res, "employer", user))
	{
		return;
	}

	try
	{
		auto j = nlohmann::json::parse(req.body);
		const long long jobId = j.value("jobId", 0LL);
		if (jobId <= 0)
		{
			res.status = 400;
			res.set_content(R"({"error":"missing job id"})", "application/json");
			return;
		}

		S_JobListing input;
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

		if (!m_service.updateEmployerJob(user, jobId, input))
		{
			res.status = 400;
			res.set_content(R"({"error":"failed to update job"})", "application/json");
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

// Handle the API endpoint for retrieving admin users based on search criteria. The function first checks if the user is authenticated and has the "admin" role. It then retrieves search parameters from the query string, such as search keyword and role filter. These parameters are passed to the getAdminUsers method of the DataService to fetch matching user profiles. The user data is formatted into a JSON response, which includes details about each user and returned to the client. If the user is not authenticated or does not have the correct role, an appropriate error response is returned.
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

// Handle the API endpoint for retrieving admin dashboard data. The function first checks if the user is authenticated and has the "admin" role. It then calls the getAdminDashboard method of the DataService to fetch various statistics and recent activity data for the admin dashboard. The data is formatted into a JSON response, which includes total counts of users, candidates, employers, jobs, applications, profiles, companies, as well as lists of recent users and recent jobs. The response is returned to the client. If the user is not authenticated or does not have the correct role, an appropriate error response is returned.
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
		{"totalApplications", data.totalApplications},
		{"totalProfiles", data.totalProfiles},
		{"totalCompanies", data.totalCompanies},
		{"recentUsers", recentUsers},
		{"recentJobs", recentJobs}
	};

	res.set_content(reply.dump(), "application/json");
}

// Handle the API endpoint for retrieving admin jobs based on search criteria. The function first checks if the user is authenticated and has the "admin" role. It then retrieves search parameters from the query string, such as search keyword and status filter. These parameters are passed to the getAdminJobs method of the DataService to fetch matching job listings. The job data is formatted into a JSON response, which includes details about each job and returned to the client. If the user is not authenticated or does not have the correct role, an appropriate error response is returned.
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
			{"posted", item.posted},
			{"applicationCount", item.applicationCount}
			});
	}

	res.set_content(nlohmann::json({ {"status", "ok"}, {"jobs", jobs} }).dump(), "application/json");
}

// Handle the API endpoint for retrieving detailed information about a specific candidate for admin view. The function first checks if the user is authenticated and has the "admin" role. It then retrieves the candidate ID from the query parameters and calls the getAdminCandidateDetails method of the DataService to fetch detailed information about the specified candidate. The candidate details are formatted into a JSON response, which includes various attributes of the candidate's profile and returned to the client. If the candidate is not found, it returns a 404 Not Found response; if the candidate ID is invalid, it returns a 400 Bad Request response. If the user is not authenticated or does not have the correct role, an appropriate error response is returned.
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

// Handle the API endpoint for retrieving detailed information about a specific employer for admin view. The function first checks if the user is authenticated and has the "admin" role. It then retrieves the employer ID from the query parameters and calls the getAdminEmployerDetails method of the DataService to fetch detailed information about the specified employer. The employer details are formatted into a JSON response, which includes various attributes of the employer's profile and returned to the client. If the employer is not found, it returns a 404 Not Found response; if the employer ID is invalid, it returns a 400 Bad Request response. If the user is not authenticated or does not have the correct role, an appropriate error response is returned.
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

// Handle the API endpoint for retrieving detailed information about a specific job for admin view. The function first checks if the user is authenticated and has the "admin" role. It then retrieves the job ID from the query parameters and calls the getAdminJobDetails method of the DataService to fetch detailed information about the specified job. The job details are formatted into a JSON response, which includes various attributes of the job and returned to the client. If the job is not found, it returns a 404 Not Found response; if the job ID is invalid, it returns a 400 Bad Request response. If the user is not authenticated or does not have the correct role, an appropriate error response is returned.
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
			{"jobDescription", job->jobDescription},
			{"applicationCount", job->applicationCount}
		};

		res.set_content(nlohmann::json({ {"status", "ok"}, {"job", payload} }).dump(), "application/json");
	}
	catch (const std::exception&)
	{
		res.status = 400;
		res.set_content(R"({"error":"invalid job id"})", "application/json");
	}
}

// Handle the API endpoint for logging out a user. The function retrieves the session token from the request cookies and, if a valid token is found, it removes the corresponding session information from the server's session management data structures. After clearing the session, it sets a response to redirect the user to the login page and clears the session cookie by setting its expiration time in the past.
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

// Handle the API endpoint for retrieving an employer's company profile. The function first checks if the user is authenticated and has the "employer" role. It then calls the getCompanyProfile method of the DataService to fetch the company profile information for the authenticated employer. The profile data is formatted into a JSON response, which includes various attributes of the company profile and returned to the client. If the user is not authenticated or does not have the correct role, an appropriate error response is returned.
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

// Handle the API endpoint for updating an employer's company profile. The function first checks if the user is authenticated and has the "employer" role. It then parses the JSON body of the request to extract various company profile details such as company name, email, phone, industry, size, location, description, and website URL. The extracted data is used to create an S_CompanyProfile object, which is then passed to the saveCompanyProfile method of the DataService along with the username to update or create the company profile in the database. If the operation is successful, it returns a JSON response indicating success; otherwise, it returns an error response. If there is an error in parsing the JSON, it returns a 400 Bad Request response.
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

// Helper function to check if the user is authenticated and has the required role. It retrieves the session token from the request cookies and checks if it corresponds to a valid session. If a valid session is found, it retrieves the username associated with the session and checks if the user's role matches the required role. If the user is not authenticated, it sets a response to redirect to the login page. If the user does not have the required role, it sets a 403 Forbidden response. The function returns true if the user is authenticated and has the required role; otherwise, it returns false.
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

// Helper function to check if the user is authenticated and has the "admin" role. It retrieves the session token from the request cookies and checks if it corresponds to a valid session. If a valid session is found, it retrieves the username associated with the session and checks if the user's role is "admin". If the user is not authenticated, it sets a response to redirect to the login page. If the user does not have the "admin" role, it sets a 403 Forbidden response. The function returns true if the user is authenticated and has the "admin" role; otherwise, it returns false.
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

// Start the server and listen for incoming requests. The function prints a message to the console indicating that the server is starting and the address and port it is listening on. It then calls the listen method of the underlying HTTP server to start accepting connections. If the server fails to start, it throws a runtime error. The function also includes catch blocks to handle any exceptions that may occur during startup and print appropriate error messages to the console.
void Server::start() 
{
	try 
	{
		std::cout << "Server starting at https://" << m_address << ":" << m_port << "\nPress Ctrl+C to stop and quit." << std::endl;
		if (!listen(m_address, m_port)) 
		{
			throw std::runtime_error("Server failed to start.");
		}
	}
	catch (const std::exception& e) 
	{
		std::cout << "Runtime error. " << e.what() << " Server not running." << std::endl;
		throw;
	}
	catch (...) 
	{
		std::cout << "Unexpected error. Server not running." << std::endl;
		throw;
	}
}

void Server::stop() 
{
	httplib::SSLServer::stop();
}

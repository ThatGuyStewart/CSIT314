#include "Database.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <ctime>
#include <exception>
#include <iostream>
#include <memory>
#include <new>
#include <random>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <nlohmann/json.hpp>


namespace
{
	bool hasColumn(const pqxx::row& row, const char* name)
	{
		for (const auto& field : row)
		{
			if (std::strcmp(field.name(), name) == 0)
			{
				return true;
			}
		}

		return false;
	}

	S_JobCard mapJobCard(const pqxx::row& row)
	{
		S_JobCard item;
		item.id = row["id"].as<long long>();	
		item.title = row["title"].as<std::string>();
		item.company = row["company"].as<std::string>();
		item.type = row["type"].as<std::string>();
		item.status = row["status"].as<std::string>();
		item.deadline = row["deadline"].as<std::string>();
		item.posted = row["posted"].as<std::string>();
		if (hasColumn(row, "location") && !row["location"].is_null()) item.location = row["location"].as<std::string>();
		if (hasColumn(row, "work_mode") && !row["work_mode"].is_null()) item.workMode = row["work_mode"].as<std::string>();
		if (hasColumn(row, "salary_range") && !row["salary_range"].is_null()) item.salaryRange = row["salary_range"].as<std::string>();
		if (hasColumn(row, "company_description") && !row["company_description"].is_null()) item.companyDescription = row["company_description"].as<std::string>();
		if (hasColumn(row, "company_website_url") && !row["company_website_url"].is_null()) item.companyWebsiteUrl = row["company_website_url"].as<std::string>();
		if (hasColumn(row, "career_level") && !row["career_level"].is_null()) item.careerLevel = row["career_level"].as<std::string>();
		if (hasColumn(row, "required_skills") && !row["required_skills"].is_null()) item.requiredSkills = row["required_skills"].as<std::string>();
		if (hasColumn(row, "required_education") && !row["required_education"].is_null()) item.requiredEducation = row["required_education"].as<std::string>();
		if (hasColumn(row, "required_experience") && !row["required_experience"].is_null()) item.requiredExperience = row["required_experience"].as<int>();
		if (hasColumn(row, "job_description") && !row["job_description"].is_null()) item.jobDescription = row["job_description"].as<std::string>();
		if (hasColumn(row, "salary_min") && !row["salary_min"].is_null()) item.salaryMin = row["salary_min"].as<std::string>();
		if (hasColumn(row, "salary_max") && !row["salary_max"].is_null()) item.salaryMax = row["salary_max"].as<std::string>();
		return item;
	}

	S_CandidateCard mapCandidateCard(const pqxx::row& row)
	{
		S_CandidateCard item;
		item.id = row["id"].as<long long>();
		item.fullName = row["full_name"].as<std::string>();
		if (hasColumn(row, "contact_info") && !row["contact_info"].is_null()) item.contactInfo = row["contact_info"].as<std::string>();
		if (hasColumn(row, "education") && !row["education"].is_null()) item.education = row["education"].as<std::string>();
		item.major = row["major"].as<std::string>();
		if (hasColumn(row, "years_experience") && !row["years_experience"].is_null()) item.yearsExperience = row["years_experience"].as<int>();
		if (hasColumn(row, "work_experience") && !row["work_experience"].is_null()) item.workExperience = row["work_experience"].as<std::string>();
		item.skills = row["skills"].as<std::string>();
		item.preferredLocation = row["preferred_location"].as<std::string>();
		item.preferredWorkMode = row["preferred_work_mode"].as<std::string>();
		if (hasColumn(row, "expected_salary") && !row["expected_salary"].is_null()) item.expectedSalary = row["expected_salary"].as<std::string>();
		if (hasColumn(row, "employment_type") && !row["employment_type"].is_null()) item.employmentType = row["employment_type"].as<std::string>();
		if (hasColumn(row, "summary") && !row["summary"].is_null()) item.summary = row["summary"].as<std::string>();
		if (hasColumn(row, "portfolio_url") && !row["portfolio_url"].is_null()) item.portfolioUrl = row["portfolio_url"].as<std::string>();
		if (hasColumn(row, "created_at") && !row["created_at"].is_null()) item.createdAt = row["created_at"].as<std::string>();
		item.experienceText = row["experience_text"].as<std::string>();
		return item;
	}
}

std::optional<S_JobCard> Database::getCandidateJobDetails(long long jobId)
{
	std::lock_guard<std::recursive_mutex> lock(m_dbMutex);
	if (!isConnected()) return std::nullopt;

	try
	{
		pqxx::read_transaction tx(*m_connection);
		const auto result = tx.exec_params(
			"SELECT "
			"    jp.id, "
			"    jp.job_title AS title, "
			"    COALESCE(cp.company_name, u.full_name) AS company, "
			"    COALESCE(cp.company_description, '') AS company_description, "
			"    COALESCE(cp.website_url, '') AS company_website_url, "
			"    jp.job_type AS type, "
			"    COALESCE(jp.career_level, '') AS career_level, "
			"    jp.status, "
			"    COALESCE(TO_CHAR(jp.application_deadline, 'YYYY-MM-DD'), '') AS deadline, "
			"    TO_CHAR(jp.created_at, 'YYYY-MM-DD') AS posted, "
			"    COALESCE(jp.job_location, '') AS location, "
			"    COALESCE(jp.work_mode, '') AS work_mode, "
			"    COALESCE(jp.required_skills, '') AS required_skills, "
			"    COALESCE(jp.required_education, '') AS required_education, "
			"    COALESCE(jp.required_experience, 0) AS required_experience, "
			"    COALESCE(jp.job_description, '') AS job_description, "
			"    COALESCE(jp.salary_min::text, '') AS salary_min, "
			"    COALESCE(jp.salary_max::text, '') AS salary_max, "
			"    CASE "
			"        WHEN jp.salary_min IS NULL AND jp.salary_max IS NULL THEN '' "
			"        WHEN jp.salary_min IS NOT NULL AND jp.salary_max IS NOT NULL THEN jp.salary_min::text || ' - ' || jp.salary_max::text "
			"        ELSE COALESCE(jp.salary_min::text, jp.salary_max::text) "
			"    END AS salary_range "
			"FROM job_postings jp "
			"INNER JOIN users u ON u.id = jp.employer_id "
			"LEFT JOIN company_profiles cp ON cp.user_id = u.id "
			"WHERE jp.id = $1 AND jp.status = 'Open' "
			"LIMIT 1",
			jobId);

		if (result.empty())
		{
			return std::nullopt;
		}

		const pqxx::row row(result[0]);
		return mapJobCard(row);
	}
	catch (const std::exception& e)
	{
		std::cout << "Get candidate job details failed: " << e.what() << std::endl;
		return std::nullopt;
	}
}

std::optional<S_CandidateCard> Database::getEmployerCandidateDetails(long long candidateId)
{
	std::lock_guard<std::recursive_mutex> lock(m_dbMutex);
	if (!isConnected()) return std::nullopt;

	try
	{
		pqxx::read_transaction tx(*m_connection);
		const auto result = tx.exec_params(
			"SELECT "
			"    u.id, "
			"    u.full_name, "
			"    COALESCE(cp.contact_info, '') AS contact_info, "
			"    COALESCE(cp.education, '') AS education, "
			"    COALESCE(cp.major, '') AS major, "
			"    COALESCE(cp.years_experience, 0) AS years_experience, "
			"    COALESCE(cp.work_experience, '') AS work_experience, "
			"    COALESCE(cp.skills, '') AS skills, "
			"    COALESCE(cp.preferred_location, '') AS preferred_location, "
			"    COALESCE(cp.preferred_work_mode, '') AS preferred_work_mode, "
			"    COALESCE(cp.expected_salary::text, '') AS expected_salary, "
			"    COALESCE(cp.employment_type, '') AS employment_type, "
			"    COALESCE(cp.summary, '') AS summary, "
			"    COALESCE(cp.portfolio_url, '') AS portfolio_url, "
			"    COALESCE(TO_CHAR(u.created_at, 'YYYY-MM-DD'), '') AS created_at, "
			"    COALESCE(cp.years_experience::text || ' years', '0 years') AS experience_text "
			"FROM users u "
			"INNER JOIN candidate_profiles cp ON cp.user_id = u.id "
			"WHERE u.role = 'candidate' AND u.id = $1 "
			"LIMIT 1",
			candidateId);

		if (result.empty())
		{
			return std::nullopt;
		}

		const pqxx::row row(result[0]);
		return mapCandidateCard(row);
	}
	catch (const std::exception& e)
	{
		std::cout << "Get employer candidate details failed: " << e.what() << std::endl;
		return std::nullopt;
	}
}

std::optional<S_CandidateProfile> Database::getCandidateProfile(const std::string& email)
{
	std::lock_guard<std::recursive_mutex> lock(m_dbMutex);
	if (!isConnected()) return std::nullopt;

	try
	{
		pqxx::read_transaction tx(*m_connection);
		const pqxx::result result = tx.exec_params(
			"SELECT "
			"    COALESCE(cp.contact_info, '') AS contact_info, "
			"    COALESCE(cp.education, '') AS education, "
			"    COALESCE(cp.major, '') AS major, "
			"    COALESCE(cp.years_experience, 0) AS years_experience, "
			"    COALESCE(cp.work_experience, '') AS work_experience, "
			"    COALESCE(cp.skills, '') AS skills, "
			"    COALESCE(cp.preferred_work_mode, '') AS preferred_work_mode, "
			"    COALESCE(cp.preferred_location, '') AS preferred_location, "
			"    COALESCE(cp.expected_salary::text, '') AS expected_salary, "
			"    COALESCE(cp.employment_type, '') AS employment_type, "
			"    COALESCE(cp.summary, '') AS summary, "
			"    COALESCE(cp.portfolio_url, '') AS portfolio_url "
			"FROM candidate_profiles cp "
			"INNER JOIN users u ON u.id = cp.user_id "
			"WHERE u.email = $1 AND u.role = 'candidate' "
			"LIMIT 1",
			email);

		if (result.empty())
		{
			return std::nullopt;
		}

		const auto& row = result[0];
		S_CandidateProfile profile;
		profile.contactInfo = row["contact_info"].as<std::string>();
		profile.education = row["education"].as<std::string>();
		profile.major = row["major"].as<std::string>();
		profile.yearsExperience = row["years_experience"].as<int>();
		profile.workExperience = row["work_experience"].as<std::string>();
		profile.skills = row["skills"].as<std::string>();
		profile.preferredWorkMode = row["preferred_work_mode"].as<std::string>();
		profile.preferredLocation = row["preferred_location"].as<std::string>();
		profile.expectedSalary = row["expected_salary"].as<std::string>();
		profile.employmentType = row["employment_type"].as<std::string>();
		profile.summary = row["summary"].as<std::string>();
		profile.portfolioUrl = row["portfolio_url"].as<std::string>();

		return profile;
	}
	catch (const std::exception& e)
	{
		std::cout << "Get candidate profile failed: " << e.what() << std::endl;
		return std::nullopt;
	}
}

std::string Database::getCandidateSkills(const std::string& email)
{
	std::lock_guard<std::recursive_mutex> lock(m_dbMutex);
	if (!isConnected()) return {};

	try
	{
		pqxx::read_transaction tx(*m_connection);
		const pqxx::result result = tx.exec_params(
			"SELECT COALESCE(cp.skills, '') AS skills "
			"FROM candidate_profiles cp "
			"INNER JOIN users u ON u.id = cp.user_id "
			"WHERE u.email = $1 AND u.role = 'candidate' "
			"LIMIT 1",
			email);

		if (result.empty() || result[0]["skills"].is_null())
		{
			return {};
		}

		return result[0]["skills"].as<std::string>();
	}
	catch (const std::exception& e)
	{
		std::cout << "Get candidate skills failed: " << e.what() << std::endl;
		return {};
	}
}

Database::Database(std::string host, std::string port, std::string dbname, std::string user, std::string password)
	: m_connection(nullptr),
	m_host(std::move(host)),
	m_port(std::move(port)),
	m_dbname(std::move(dbname)),
	m_user(std::move(user)),
	m_password(std::move(password))
{
	if (!createDatabaseIfMissing())
	{
		throw std::runtime_error("Database creation failed.");
	}

	if (!connect(m_host, m_port, m_dbname, m_user, m_password))
	{
		throw std::runtime_error("Database connection failed.");
	}

	if (!createSchema())
	{
		throw std::runtime_error("Database schema creation failed.");
	}

}

std::string Database::loadSqlFile(const std::string& path)
{
	std::ifstream file(path, std::ios::binary);
	if (!file)
	{
		throw std::runtime_error("Failed to open SQL file: " + path);
	}

	std::ostringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

bool Database::createDatabaseIfMissing()
{
	try
	{
		pqxx::connection adminConnection(
			"host=" + m_host +
			" port=" + m_port +
			" dbname=postgres" +
			" user=" + m_user +
			" password=" + m_password);

		pqxx::nontransaction tx(adminConnection);

		pqxx::result result = tx.exec_params(
			"SELECT 1 FROM pg_database WHERE datname = $1",
			m_dbname);

		if (result.empty())
		{
			tx.exec("CREATE DATABASE " + tx.quote_name(m_dbname));
		}

		return true;
	}
	catch (const std::exception& e)
	{
		std::cerr << "Create database failed: " << e.what() << std::endl;
		return false;
	}
}

bool Database::createSchema()
{
	std::lock_guard<std::recursive_mutex> lock(m_dbMutex);

	try
	{
		const std::string sql = loadSqlFile("database/schema.sql");

		pqxx::work tx(*m_connection);
		tx.exec0(sql);
		tx.exec0(
			"ALTER TABLE job_postings ADD COLUMN IF NOT EXISTS company_info VARCHAR(200);"
			"ALTER TABLE job_postings ADD COLUMN IF NOT EXISTS job_description TEXT;"
			"ALTER TABLE job_postings ADD COLUMN IF NOT EXISTS required_education VARCHAR(20) DEFAULT 'Any';"
			"ALTER TABLE job_postings ADD COLUMN IF NOT EXISTS required_skills TEXT;"
			"ALTER TABLE job_postings ADD COLUMN IF NOT EXISTS required_experience INT NOT NULL DEFAULT 0;"
			"ALTER TABLE job_postings ADD COLUMN IF NOT EXISTS work_mode VARCHAR(20) DEFAULT 'On-site';"
			"ALTER TABLE job_postings ADD COLUMN IF NOT EXISTS job_location VARCHAR(150);"
			"ALTER TABLE job_postings ADD COLUMN IF NOT EXISTS salary_min NUMERIC(10,2);"
			"ALTER TABLE job_postings ADD COLUMN IF NOT EXISTS salary_max NUMERIC(10,2);"
			"ALTER TABLE job_postings ADD COLUMN IF NOT EXISTS job_type VARCHAR(20) DEFAULT 'Full-time';"
			"ALTER TABLE job_postings ADD COLUMN IF NOT EXISTS career_level VARCHAR(20) DEFAULT 'Entry-level';"
			"ALTER TABLE job_postings ADD COLUMN IF NOT EXISTS application_deadline DATE;"
			"ALTER TABLE job_postings ADD COLUMN IF NOT EXISTS status VARCHAR(20) DEFAULT 'Open';"
			"CREATE TABLE IF NOT EXISTS candidate_applications ("
			"    id BIGSERIAL PRIMARY KEY,"
			"    candidate_id BIGINT NOT NULL REFERENCES users(id) ON DELETE CASCADE,"
			"    job_posting_id BIGINT NOT NULL REFERENCES job_postings(id) ON DELETE CASCADE,"
			"    created_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP,"
			"    UNIQUE (candidate_id, job_posting_id)"
			");"
			"CREATE INDEX IF NOT EXISTS idx_candidate_applications_candidate_id ON candidate_applications(candidate_id);"
			"CREATE INDEX IF NOT EXISTS idx_candidate_applications_job_posting_id ON candidate_applications(job_posting_id);"
		);
		tx.commit();

		return true;
	}
	catch (const std::exception& e)
	{
		std::cerr << "Database schema creation failed: " << e.what() << std::endl;
		return false;
	}
}

bool Database::createCandidateApplication(const std::string& email, long long jobId)
{
	std::lock_guard<std::recursive_mutex> lock(m_dbMutex);
	if (!isConnected() || jobId <= 0) return false;

	try
	{
		pqxx::work tx(*m_connection);
		const auto result = tx.exec_params(
			"INSERT INTO candidate_applications (candidate_id, job_posting_id) "
			"SELECT u.id, jp.id "
			"FROM users u "
			"INNER JOIN job_postings jp ON jp.id = $2 "
			"WHERE u.email = $1 AND u.role = 'candidate' AND jp.status = 'Open' "
			"ON CONFLICT (candidate_id, job_posting_id) DO NOTHING "
			"RETURNING id",
			email,
			jobId);

		tx.commit();
		return !result.empty() || hasCandidateAppliedToJob(email, jobId);
	}
	catch (const std::exception& e)
	{
		std::cout << "Create candidate application failed: " << e.what() << std::endl;
		return false;
	}
}

bool Database::hasCandidateAppliedToJob(const std::string& email, long long jobId)
{
	std::lock_guard<std::recursive_mutex> lock(m_dbMutex);
	if (!isConnected() || jobId <= 0) return false;

	try
	{
		pqxx::read_transaction tx(*m_connection);
		const auto result = tx.exec_params(
			"SELECT 1 "
			"FROM candidate_applications ca "
			"INNER JOIN users u ON u.id = ca.candidate_id "
			"WHERE u.email = $1 AND u.role = 'candidate' AND ca.job_posting_id = $2 "
			"LIMIT 1",
			email,
			jobId);

		return !result.empty();
	}
	catch (const std::exception& e)
	{
		std::cout << "Check candidate application failed: " << e.what() << std::endl;
		return false;
	}
}

bool Database::connect(std::string& host, std::string& port, std::string& dbname, std::string& user, std::string& password)
{
	std::lock_guard<std::recursive_mutex> lock(m_dbMutex);

	try
	{
		m_connection = std::make_unique<pqxx::connection>("host=" + host + " port=" + port + " dbname=" + dbname + " user=" + user + " password=" + password);
		std::cout << "Database connection established." << std::endl;
		return m_connection->is_open();
	}
	catch (const std::exception& e)
	{
		std::cerr << "Database connection failed: " << e.what() << std::endl;
		m_connection.reset();
		return false;
	}
}

bool Database::isConnected() const
{
	std::lock_guard<std::recursive_mutex> lock(m_dbMutex);
	return (m_connection != nullptr && m_connection->is_open());
}

bool Database::createAccount(const std::string& fullName, const std::string& email, const std::string& password, const std::string& role)
{
	std::lock_guard<std::recursive_mutex> lock(m_dbMutex);
	if (!isConnected()) return false;
	if (accountExists(email)) return false;

	if (fullName.empty() || email.empty() || password.empty())
	{
		return false;
	}

	if (role != "candidate" && role != "employer" && role != "admin")
	{
		return false;
	}

	try
	{
		pqxx::work tx(*m_connection);

		tx.exec_params(
			"INSERT INTO users (full_name, email, password_hash, role) "
			"VALUES ($1, $2, $3, $4)",
			fullName,
			email,
			password,
			role);

		tx.commit();
		return true;
	}
	catch (const std::exception& e)
	{
		std::cout << "Create account failed: " << e.what() << std::endl;
		return false;
	}
}

bool Database::updateMembershipStatus(const std::string& email, const std::string& membershipStatus)
{
	std::lock_guard<std::recursive_mutex> lock(m_dbMutex);
	if (!isConnected()) return false;

	if (membershipStatus != "free" && membershipStatus != "premium")
	{
		return false;
	}

	try
	{
		pqxx::work tx(*m_connection);
		const auto result = tx.exec_params(
			"UPDATE users SET membership_status = $1 WHERE email = $2 AND role IN ('candidate', 'employer')",
			membershipStatus,
			email);
		tx.commit();
		return result.affected_rows() > 0;
	}
	catch (const std::exception& e)
	{
		std::cout << "Update membership status failed: " << e.what() << std::endl;
		return false;
	}
}

bool Database::validateAccount(const std::string& email, const std::string& password)
{
	std::lock_guard<std::recursive_mutex> lock(m_dbMutex);
	if (!isConnected()) return false;

	try
	{
		pqxx::read_transaction tx(*m_connection);
		pqxx::result result = tx.exec_params(
			"SELECT password_hash "
			"FROM users "
			"WHERE email = $1",
			email);

		if (result.empty())
		{
			return false;
		}

		const std::string storedPassword = result[0]["password_hash"].as<std::string>();
		return storedPassword == password;
	}
	catch (const std::exception& e)
	{
		std::cout << "Validate account failed: " << e.what() << std::endl;
		return false;
	}
}

bool Database::accountExists(const std::string& email)
{
	std::lock_guard<std::recursive_mutex> lock(m_dbMutex);
	if (!isConnected()) return false;

	try
	{
		pqxx::read_transaction tx(*m_connection);
		pqxx::result result = tx.exec_params(
			"SELECT 1 "
			"FROM users "
			"WHERE email = $1 "
			"LIMIT 1",
			email);

		return !result.empty();
	}
	catch (const std::exception& e)
	{
		std::cout << "Account exists check failed: " << e.what() << std::endl;
		return false;
	}
}

bool Database::isAdminAccount(const std::string& email)
{
	std::lock_guard<std::recursive_mutex> lock(m_dbMutex);
	if (!isConnected()) return false;

	try
	{
		pqxx::read_transaction tx(*m_connection);
		pqxx::result result = tx.exec_params(
			"SELECT role "
			"FROM users "
			"WHERE email = $1",
			email);

		if (result.empty())
		{
			return false;
		}

		return result[0]["role"].as<std::string>() == "admin";
	}
	catch (const std::exception& e)
	{
		std::cout << "Admin account check failed: " << e.what() << std::endl;
		return false;
	}
}

std::optional<std::string> Database::getAccountRole(const std::string& email)
{
	std::lock_guard<std::recursive_mutex> lock(m_dbMutex);
	if (!isConnected()) return std::nullopt;

	try
	{
		pqxx::read_transaction tx(*m_connection);
		pqxx::result result = tx.exec_params(
			"SELECT role "
			"FROM users "
			"WHERE email = $1",
			email);

		if (result.empty())
		{
			return std::nullopt;
		}

		return result[0]["role"].as<std::string>();
	}
	catch (const std::exception& e)
	{
		std::cout << "Get account role failed: " << e.what() << std::endl;
		return std::nullopt;
	}
}

bool Database::saveCandidateProfile(const std::string& email, const S_CandidateProfile& input)
{
	std::lock_guard<std::recursive_mutex> lock(m_dbMutex);
	if (!isConnected()) return false;

	try
	{
		pqxx::work tx(*m_connection);

		const pqxx::result result = tx.exec_params(
			"INSERT INTO candidate_profiles ("
			"    user_id, contact_info, education, major, years_experience, work_experience, skills,"
			"    preferred_work_mode, preferred_location, expected_salary, employment_type, summary, portfolio_url"
			") "
			"SELECT "
			"    u.id, $2, $3, $4, $5, $6, $7, $8, $9, NULLIF($10, '')::numeric, $11, $12, $13 "
			"FROM users u "
			"WHERE u.email = $1 AND u.role = 'candidate' "
			"ON CONFLICT (user_id) DO UPDATE SET "
			"    contact_info = EXCLUDED.contact_info, "
			"    education = EXCLUDED.education, "
			"    major = EXCLUDED.major, "
			"    years_experience = EXCLUDED.years_experience, "
			"    work_experience = EXCLUDED.work_experience, "
			"    skills = EXCLUDED.skills, "
			"    preferred_work_mode = EXCLUDED.preferred_work_mode, "
			"    preferred_location = EXCLUDED.preferred_location, "
			"    expected_salary = EXCLUDED.expected_salary, "
			"    employment_type = EXCLUDED.employment_type, "
			"    summary = EXCLUDED.summary, "
			"    portfolio_url = EXCLUDED.portfolio_url, "
			"    updated_at = CURRENT_TIMESTAMP "
			"RETURNING user_id",
			email,
			input.contactInfo,
			input.education,
			input.major,
			input.yearsExperience,
			input.workExperience,
			input.skills,
			input.preferredWorkMode,
			input.preferredLocation,
			input.expectedSalary,
			input.employmentType,
			input.summary,
			input.portfolioUrl);

		if (result.empty())
		{
			return false;
		}

		tx.commit();
		return true;
	}
	catch (const std::exception& e)
	{
		std::cout << "Save candidate profile failed: " << e.what() << std::endl;
		return false;
	}
}

bool Database::createJobPosting(const std::string& email, const S_JobListing& input)
{
	std::lock_guard<std::recursive_mutex> lock(m_dbMutex);
	if (!isConnected()) return false;
	if (input.jobTitle.empty() || input.jobDescription.empty()) return false;

	try
	{
		pqxx::work tx(*m_connection);

		const pqxx::result result = tx.exec_params(
			"INSERT INTO job_postings ("
			"    employer_id, company_info, job_title, job_description, required_education, required_skills,"
			"    required_experience, work_mode, job_location, salary_min, salary_max, job_type,"
			"    career_level, application_deadline, status"
			") "
			"SELECT "
			"    u.id, $2, $3, $4, $5, $6, $7, $8, $9, "
			"    NULLIF($10, '')::numeric, NULLIF($11, '')::numeric, $12, $13, NULLIF($14, '')::date, $15 "
			"FROM users u "
			"WHERE u.email = $1 AND u.role = 'employer' "
			"RETURNING id",
			email,
			input.companyInfo,
			input.jobTitle,
			input.jobDescription,
			input.requiredEducation,
			input.requiredSkills,
			input.requiredExperience,
			input.workMode,
			input.jobLocation,
			input.salaryMin,
			input.salaryMax,
			input.jobType,
			input.careerLevel,
			input.applicationDeadline,
			input.status);

		if (result.empty())
		{
			return false;
		}

		tx.commit();
		return true;
	}
	catch (const std::exception& e)
	{
		std::cout << "Create employer job posting failed: " << e.what() << std::endl;
		return false;
	}
}

std::vector<S_UserSummary> Database::getAdminUsers(const std::string& role)
{
	std::lock_guard<std::recursive_mutex> lock(m_dbMutex);
	std::vector<S_UserSummary> users;
	if (!isConnected()) return users;

	try
	{
		pqxx::read_transaction tx(*m_connection);
		pqxx::result result = tx.exec_params(
			"SELECT "
			"    u.id, "
			"    u.full_name, "
			"    COALESCE(cp_company.company_name, '') AS company_name, "
			"    u.email, "
			"    u.role, "
			"    u.membership_status, "
			"    CASE "
			"        WHEN u.role = 'candidate' THEN CASE WHEN cp.user_id IS NOT NULL THEN 'Profile' ELSE 'No Profile' END "
			"        WHEN u.role = 'employer' THEN CONCAT(COUNT(jp.id), ' Jobs') "
			"        ELSE '-' "
			"    END AS profile_or_jobs, "
			"    TO_CHAR(u.created_at, 'YYYY-MM-DD') AS created_at "
			"FROM users u "
			"LEFT JOIN candidate_profiles cp ON cp.user_id = u.id "
			"LEFT JOIN company_profiles cp_company ON cp_company.user_id = u.id "
			"LEFT JOIN job_postings jp ON jp.employer_id = u.id "
			"WHERE ($1 = '' OR u.role = $1) "
			"GROUP BY u.id, u.full_name, cp_company.company_name, u.email, u.role, u.membership_status, cp.user_id, u.created_at "
			"ORDER BY u.created_at DESC, u.id DESC",
			role);

		users.reserve(result.size());
		for (const auto& row : result)
		{
			S_UserSummary item;
			item.id = row["id"].as<long long>();
			item.fullName = row["full_name"].as<std::string>();
			item.companyName = row["company_name"].as<std::string>();
			item.email = row["email"].as<std::string>();
			item.role = row["role"].as<std::string>();
			item.membershipStatus = row["membership_status"].as<std::string>();
			item.profileOrJobs = row["profile_or_jobs"].as<std::string>();
			item.createdAt = row["created_at"].as<std::string>();
			users.push_back(std::move(item));
		}

		return users;
	}
	catch (const std::exception& e)
	{
		std::cout << "Get admin users failed: " << e.what() << std::endl;
		return {};
	}
}

S_CandidateDashboardData Database::getCandidateDashboard(const std::string& email)
{
	std::lock_guard<std::recursive_mutex> lock(m_dbMutex);
	S_CandidateDashboardData data;
	if (!isConnected()) return data;

	try
	{
		{
			pqxx::read_transaction tx(*m_connection);

			const auto membership = tx.exec_params(
				"SELECT membership_status FROM users WHERE email = $1 AND role = 'candidate'",
				email);

			if (!membership.empty())
			{
				data.membershipStatus = membership[0]["membership_status"].as<std::string>();
			}

			const auto jobsCount = tx.exec(
				"SELECT COUNT(*) AS count FROM job_postings WHERE status = 'Open'");
			if (!jobsCount.empty())
			{
				data.openPositions = jobsCount[0]["count"].as<int>();
			}

			const auto profile = tx.exec_params(
				"SELECT contact_info, education, major, years_experience, work_experience, skills, "
				"preferred_work_mode, preferred_location, expected_salary, employment_type, summary, portfolio_url "
				"FROM candidate_profiles cp "
				"INNER JOIN users u ON u.id = cp.user_id "
				"WHERE u.email = $1",
				email);

			if (!profile.empty())
			{
				const auto& row = profile[0];
				int complete = 0;
				int total = 11;

				if (!row["contact_info"].is_null() && !row["contact_info"].as<std::string>().empty()) ++complete;
				if (!row["education"].is_null() && !row["education"].as<std::string>().empty()) ++complete;
				if (!row["major"].is_null() && !row["major"].as<std::string>().empty()) ++complete;
				if (!row["work_experience"].is_null() && !row["work_experience"].as<std::string>().empty()) ++complete;
				if (!row["skills"].is_null() && !row["skills"].as<std::string>().empty()) ++complete;
				if (!row["preferred_work_mode"].is_null() && !row["preferred_work_mode"].as<std::string>().empty()) ++complete;
				if (!row["preferred_location"].is_null() && !row["preferred_location"].as<std::string>().empty()) ++complete;
				if (!row["expected_salary"].is_null()) ++complete;
				if (!row["employment_type"].is_null() && !row["employment_type"].as<std::string>().empty()) ++complete;
				if (!row["summary"].is_null() && !row["summary"].as<std::string>().empty()) ++complete;
				if (!row["portfolio_url"].is_null() && !row["portfolio_url"].as<std::string>().empty()) ++complete;

				data.profileComplete = (complete * 100) / total;
			}
		}

		return data;
	}
	catch (const std::exception& e)
	{
		std::cout << "Get candidate dashboard failed: " << e.what() << std::endl;
		return {};
	}
}

std::vector<S_JobCard> Database::getCandidateJobs()
{
	std::lock_guard<std::recursive_mutex> lock(m_dbMutex);
	std::vector<S_JobCard> jobs;
	if (!isConnected()) return jobs;

	try
	{
		pqxx::read_transaction tx(*m_connection);
		const auto result = tx.exec(
			"SELECT "
			"    jp.id, "
			"    jp.job_title AS title, "
			"    COALESCE(cp.company_name, u.full_name) AS company, "
			"    jp.job_type AS type, "
			"    jp.status, "
			"    COALESCE(TO_CHAR(jp.application_deadline, 'YYYY-MM-DD'), '') AS deadline, "
			"    TO_CHAR(jp.created_at, 'YYYY-MM-DD') AS posted, "
			"    COALESCE(jp.job_location, '') AS location, "
			"    COALESCE(jp.work_mode, '') AS work_mode, "
			"    COALESCE(jp.required_skills, '') AS required_skills, "
			"    COALESCE(jp.required_education, '') AS required_education, "
			"    COALESCE(jp.required_experience, 0) AS required_experience, "
			"    COALESCE(jp.job_description, '') AS job_description, "
			"    COALESCE(jp.salary_min::text, '') AS salary_min, "
			"    COALESCE(jp.salary_max::text, '') AS salary_max, "
			"    CASE "
			"        WHEN jp.salary_min IS NULL AND jp.salary_max IS NULL THEN '' "
			"        WHEN jp.salary_min IS NOT NULL AND jp.salary_max IS NOT NULL THEN jp.salary_min::text || ' - ' || jp.salary_max::text "
			"        ELSE COALESCE(jp.salary_min::text, jp.salary_max::text) "
			"    END AS salary_range "
			"FROM job_postings jp "
			"INNER JOIN users u ON u.id = jp.employer_id "
			"LEFT JOIN company_profiles cp ON cp.user_id = u.id "
			"WHERE jp.status = 'Open' "
			"ORDER BY jp.created_at DESC");

		jobs.reserve(result.size());
		for (const auto& row : result)
		{
			jobs.push_back(mapJobCard(pqxx::row(row)));
		}

		return jobs;
	}
	catch (const std::exception& e)
	{
		std::cout << "Get candidate jobs failed: " << e.what() << std::endl;
		return {};
	}
}

std::vector<S_JobCard> Database::getFilteredCandidateJobs(const std::string& location, const std::string& workMode, const std::string& jobType, const std::string& careerLevel, const std::string& salaryMin, const std::string& salaryMax)
{
	std::lock_guard<std::recursive_mutex> lock(m_dbMutex);
	std::vector<S_JobCard> jobs;
	if (!isConnected()) return jobs;

	try
	{
		pqxx::read_transaction tx(*m_connection);
		const auto result = tx.exec_params(
			"SELECT "
			"    jp.id, "
			"    jp.job_title AS title, "
			"    COALESCE(cp.company_name, u.full_name) AS company, "
			"    jp.job_type AS type, "
			"    jp.status, "
			"    COALESCE(TO_CHAR(jp.application_deadline, 'YYYY-MM-DD'), '') AS deadline, "
			"    TO_CHAR(jp.created_at, 'YYYY-MM-DD') AS posted, "
			"    COALESCE(jp.job_location, '') AS location, "
			"    COALESCE(jp.work_mode, '') AS work_mode, "
			"    COALESCE(jp.required_skills, '') AS required_skills, "
			"    COALESCE(jp.required_education, '') AS required_education, "
			"    COALESCE(jp.required_experience, 0) AS required_experience, "
			"    COALESCE(jp.job_description, '') AS job_description, "
			"    COALESCE(jp.salary_min::text, '') AS salary_min, "
			"    COALESCE(jp.salary_max::text, '') AS salary_max, "
			"    CASE "
			"        WHEN jp.salary_min IS NULL AND jp.salary_max IS NULL THEN '' "
			"        WHEN jp.salary_min IS NOT NULL AND jp.salary_max IS NOT NULL THEN jp.salary_min::text || ' - ' || jp.salary_max::text "
			"        ELSE COALESCE(jp.salary_min::text, jp.salary_max::text) "
			"    END AS salary_range "
			"FROM job_postings jp "
			"INNER JOIN users u ON u.id = jp.employer_id "
			"LEFT JOIN company_profiles cp ON cp.user_id = u.id "
			"WHERE jp.status = 'Open' "
			"  AND ($1 = '' OR jp.job_location ILIKE '%' || $1 || '%') "
			"  AND ($2 = '' OR jp.work_mode = $2) "
			"  AND ($3 = '' OR jp.job_type = $3) "
			"  AND ($4 = '' OR jp.career_level = $4) "
			"  AND ($5 = '' OR COALESCE(jp.salary_max, jp.salary_min) >= NULLIF($5, '')::numeric) "
			"  AND ($6 = '' OR COALESCE(jp.salary_min, jp.salary_max) <= NULLIF($6, '')::numeric) "
			"ORDER BY jp.created_at DESC",
			location,
			workMode,
			jobType,
			careerLevel,
			salaryMin,
			salaryMax);

		jobs.reserve(result.size());
		for (const auto& row : result)
		{
			jobs.push_back(mapJobCard(pqxx::row(row)));
		}

		return jobs;
	}
	catch (const std::exception& e)
	{
		std::cout << "Get filtered candidate jobs failed: " << e.what() << std::endl;
		return {};
	}
}
S_EmployerDashboardData Database::getEmployerDashboard(const std::string& email)
{
	std::lock_guard<std::recursive_mutex> lock(m_dbMutex);
	S_EmployerDashboardData data;
	if (!isConnected()) return data;

	try
	{
		pqxx::read_transaction tx(*m_connection);

		const auto membership = tx.exec_params(
			"SELECT membership_status FROM users WHERE email = $1 AND role = 'employer'",
			email);
		if (!membership.empty())
		{
			data.membershipStatus = membership[0]["membership_status"].as<std::string>();
		}

		const auto postCounts = tx.exec_params(
			"SELECT "
			"    COUNT(*)::int AS total_posts, "
			"    SUM(CASE WHEN status = 'Open' THEN 1 ELSE 0 END)::int AS active_posts "
			"FROM job_postings jp "
			"INNER JOIN users u ON u.id = jp.employer_id "
			"WHERE u.email = $1",
			email);

		if (!postCounts.empty())
		{
			data.totalPosts = postCounts[0]["total_posts"].is_null() ? 0 : postCounts[0]["total_posts"].as<int>();
			data.activeCount = postCounts[0]["active_posts"].is_null() ? 0 : postCounts[0]["active_posts"].as<int>();
		}

		const auto candidatesCount = tx.exec(
			"SELECT COUNT(*)::int AS total_candidates FROM users WHERE role = 'candidate'");
		if (!candidatesCount.empty())
		{
			data.totalCandidates = candidatesCount[0]["total_candidates"].as<int>();
		}

		const auto topCandidatesResult = tx.exec(
			"SELECT "
			"    u.id, "
			"    u.full_name, "
			"    COALESCE(cp.major, '') AS major, "
			"    COALESCE(cp.skills, '') AS skills, "
			"    COALESCE(cp.preferred_location, '') AS preferred_location, "
			"    COALESCE(cp.preferred_work_mode, '') AS preferred_work_mode, "
			"    cp.years_experience::text || ' years' AS experience_text "
			"FROM users u "
			"INNER JOIN candidate_profiles cp ON cp.user_id = u.id "
			"WHERE u.role = 'candidate' "
			"ORDER BY cp.years_experience DESC, u.created_at DESC "
			"LIMIT 3");

		for (const auto& row : topCandidatesResult)
		{
			data.topCandidates.push_back(mapCandidateCard(pqxx::row(row)));
		}

		return data;
	}
	catch (const std::exception& e)
	{
		std::cout << "Get employer dashboard failed: " << e.what() << std::endl;
		return {};
	}
}

std::vector<S_CandidateCard> Database::getEmployerCandidates()
{
	std::lock_guard<std::recursive_mutex> lock(m_dbMutex);
	std::vector<S_CandidateCard> candidates;
	if (!isConnected()) return candidates;

	try
	{
		pqxx::read_transaction tx(*m_connection);
		const auto result = tx.exec(
			"SELECT "
			"    u.id, "
			"    u.full_name, "
			"    COALESCE(cp.contact_info, '') AS contact_info, "
			"    COALESCE(cp.education, '') AS education, "
			"    COALESCE(cp.major, '') AS major, "
			"    COALESCE(cp.years_experience, 0) AS years_experience, "
			"    COALESCE(cp.work_experience, '') AS work_experience, "
			"    COALESCE(cp.skills, '') AS skills, "
			"    COALESCE(cp.preferred_location, '') AS preferred_location, "
			"    COALESCE(cp.preferred_work_mode, '') AS preferred_work_mode, "
			"    COALESCE(cp.expected_salary::text, '') AS expected_salary, "
			"    COALESCE(cp.employment_type, '') AS employment_type, "
			"    COALESCE(cp.summary, '') AS summary, "
			"    COALESCE(cp.portfolio_url, '') AS portfolio_url, "
			"    COALESCE(TO_CHAR(u.created_at, 'YYYY-MM-DD'), '') AS created_at, "
			"    COALESCE(cp.years_experience::text || ' years', '0 years') AS experience_text "
			"FROM users u "
			"INNER JOIN candidate_profiles cp ON cp.user_id = u.id "
			"WHERE u.role = 'candidate' "
			"ORDER BY cp.years_experience DESC, u.created_at DESC");

		candidates.reserve(result.size());
		for (const auto& row : result)
		{
			candidates.push_back(mapCandidateCard(pqxx::row(row)));
		}

		return candidates;
	}
	catch (const std::exception& e)
	{
		std::cout << "Get employer candidates failed: " << e.what() << std::endl;
		return {};
	}
}

std::vector<S_CandidateCard> Database::getFilteredEmployerCandidates(const std::string& education, const std::string& yearsExperience, const std::string& preferredWorkMode, const std::string& preferredLocation)
{
	std::lock_guard<std::recursive_mutex> lock(m_dbMutex);
	std::vector<S_CandidateCard> candidates;
	if (!isConnected()) return candidates;

	try
	{
		pqxx::read_transaction tx(*m_connection);
		const auto result = tx.exec_params(
			"SELECT "
			"    u.id, "
			"    u.full_name, "
			"    COALESCE(cp.contact_info, '') AS contact_info, "
			"    COALESCE(cp.education, '') AS education, "
			"    COALESCE(cp.major, '') AS major, "
			"    COALESCE(cp.years_experience, 0) AS years_experience, "
			"    COALESCE(cp.work_experience, '') AS work_experience, "
			"    COALESCE(cp.skills, '') AS skills, "
			"    COALESCE(cp.preferred_location, '') AS preferred_location, "
			"    COALESCE(cp.preferred_work_mode, '') AS preferred_work_mode, "
			"    COALESCE(cp.expected_salary::text, '') AS expected_salary, "
			"    COALESCE(cp.employment_type, '') AS employment_type, "
			"    COALESCE(cp.summary, '') AS summary, "
			"    COALESCE(cp.portfolio_url, '') AS portfolio_url, "
			"    COALESCE(TO_CHAR(u.created_at, 'YYYY-MM-DD'), '') AS created_at, "
			"    COALESCE(cp.years_experience::text || ' years', '0 years') AS experience_text "
			"FROM users u "
			"INNER JOIN candidate_profiles cp ON cp.user_id = u.id "
			"WHERE u.role = 'candidate' "
			"  AND ($1 = '' OR cp.education ILIKE '%' || $1 || '%') "
			"  AND ($2 = '' OR cp.years_experience >= NULLIF($2, '')::int) "
			"  AND ($3 = '' OR cp.preferred_work_mode = $3) "
			"  AND ($4 = '' OR cp.preferred_location ILIKE '%' || $4 || '%') "
			"ORDER BY cp.years_experience DESC, u.created_at DESC",
			education,
			yearsExperience,
			preferredWorkMode,
			preferredLocation);

		candidates.reserve(result.size());
		for (const auto& row : result)
		{
			candidates.push_back(mapCandidateCard(pqxx::row(row)));
		}

		return candidates;
	}
	catch (const std::exception& e)
	{
		std::cout << "Get filtered employer candidates failed: " << e.what() << std::endl;
		return {};
	}
}

std::vector<S_JobCard> Database::getEmployerJobs(const std::string& email)
{
	std::lock_guard<std::recursive_mutex> lock(m_dbMutex);
	std::vector<S_JobCard> jobs;
	if (!isConnected()) return jobs;

	try
	{
		pqxx::read_transaction tx(*m_connection);
		const auto result = tx.exec_params(
			"SELECT "
			"    jp.id, "
			"    jp.job_title AS title, "
			"    COALESCE(cp.company_name, u.full_name) AS company, "
			"    jp.job_type AS type, "
			"    COALESCE(jp.career_level, '') AS career_level, "
			"    jp.status, "
			"    COALESCE(TO_CHAR(jp.application_deadline, 'YYYY-MM-DD'), '') AS deadline, "
			"    TO_CHAR(jp.created_at, 'YYYY-MM-DD') AS posted, "
			"    COALESCE(jp.job_location, '') AS location, "
			"    COALESCE(jp.work_mode, '') AS work_mode, "
			"    COALESCE(jp.required_skills, '') AS required_skills, "
			"    COALESCE(jp.required_education, '') AS required_education, "
			"    COALESCE(jp.required_experience, 0) AS required_experience, "
			"    COALESCE(jp.job_description, '') AS job_description, "
			"    COALESCE(jp.salary_min::text, '') AS salary_min, "
			"    COALESCE(jp.salary_max::text, '') AS salary_max, "
			"    CASE "
			"        WHEN jp.salary_min IS NULL AND jp.salary_max IS NULL THEN '' "
			"        WHEN jp.salary_min IS NOT NULL AND jp.salary_max IS NOT NULL THEN jp.salary_min::text || ' - ' || jp.salary_max::text "
			"        ELSE COALESCE(jp.salary_min::text, jp.salary_max::text) "
			"    END AS salary_range "
			"FROM job_postings jp "
			"INNER JOIN users u ON u.id = jp.employer_id "
			"LEFT JOIN company_profiles cp ON cp.user_id = u.id "
			"WHERE u.email = $1 "
			"ORDER BY jp.created_at DESC",
			email);

		for (const auto& row : result)
		{
			jobs.push_back(mapJobCard(pqxx::row(row)));
		}

		return jobs;
	}
	catch (const std::exception& e)
	{
		std::cout << "Get employer jobs failed: " << e.what() << std::endl;
		return {};
	}
}

S_AdminDashboardData Database::getAdminDashboard()
{
	std::lock_guard<std::recursive_mutex> lock(m_dbMutex);
	S_AdminDashboardData data;
	if (!isConnected()) return data;

	try
	{
		pqxx::read_transaction tx(*m_connection);

		const auto users = tx.exec(
			"SELECT "
			"COUNT(*)::int AS total_users, "
			"SUM(CASE WHEN role = 'candidate' THEN 1 ELSE 0 END)::int AS total_candidates, "
			"SUM(CASE WHEN role = 'employer' THEN 1 ELSE 0 END)::int AS total_employers, "
			"SUM(CASE WHEN membership_status = 'premium' THEN 1 ELSE 0 END)::int AS premium_users "
			"FROM users");

		if (!users.empty())
		{
			data.totalUsers = users[0]["total_users"].is_null() ? 0 : users[0]["total_users"].as<int>();
			data.totalCandidates = users[0]["total_candidates"].is_null() ? 0 : users[0]["total_candidates"].as<int>();
			data.totalEmployers = users[0]["total_employers"].is_null() ? 0 : users[0]["total_employers"].as<int>();
			data.premiumUsers = users[0]["premium_users"].is_null() ? 0 : users[0]["premium_users"].as<int>();
		}

		const auto jobs = tx.exec(
			"SELECT "
			"COUNT(*)::int AS total_jobs, "
			"SUM(CASE WHEN status = 'Open' THEN 1 ELSE 0 END)::int AS open_jobs "
			"FROM job_postings");

		if (!jobs.empty())
		{
			data.totalJobs = jobs[0]["total_jobs"].is_null() ? 0 : jobs[0]["total_jobs"].as<int>();
			data.openJobs = jobs[0]["open_jobs"].is_null() ? 0 : jobs[0]["open_jobs"].as<int>();
		}

		const auto profiles = tx.exec(
			"SELECT "
			"(SELECT COUNT(*)::int FROM candidate_profiles) AS total_profiles, "
			"(SELECT COUNT(*)::int FROM company_profiles) AS total_companies");

		if (!profiles.empty())
		{
			data.totalProfiles = profiles[0]["total_profiles"].is_null() ? 0 : profiles[0]["total_profiles"].as<int>();
			data.totalCompanies = profiles[0]["total_companies"].is_null() ? 0 : profiles[0]["total_companies"].as<int>();
		}

		const auto recentUsersResult = tx.exec(
			"SELECT "
			"u.id, u.full_name, COALESCE(cp_company.company_name, '') AS company_name, u.email, u.role, u.membership_status, "
			"CASE "
			"    WHEN u.role = 'candidate' THEN CASE WHEN cp.user_id IS NOT NULL THEN 'Profile' ELSE 'No Profile' END "
			"    WHEN u.role = 'employer' THEN COALESCE(COUNT(jp.id)::text || ' Jobs', '0 Jobs') "
			"    ELSE '-' "
			"END AS profile_or_jobs, "
			"TO_CHAR(u.created_at, 'YYYY-MM-DD') AS created_at "
			"FROM users u "
			"LEFT JOIN candidate_profiles cp ON cp.user_id = u.id "
			"LEFT JOIN company_profiles cp_company ON cp_company.user_id = u.id "
			"LEFT JOIN job_postings jp ON jp.employer_id = u.id "
			"GROUP BY u.id, u.full_name, cp_company.company_name, u.email, u.role, u.membership_status, cp.user_id, u.created_at "
			"ORDER BY u.created_at DESC "
			"LIMIT 5");

		for (const auto& row : recentUsersResult)
		{
			S_UserSummary item;
			item.id = row["id"].as<long long>();
			item.fullName = row["full_name"].as<std::string>();
			item.companyName = row["company_name"].as<std::string>();
			item.email = row["email"].as<std::string>();
			item.role = row["role"].as<std::string>();
			item.membershipStatus = row["membership_status"].as<std::string>();
			item.profileOrJobs = row["profile_or_jobs"].as<std::string>();
			item.createdAt = row["created_at"].as<std::string>();
			data.recentUsers.push_back(std::move(item));
		}

		const auto recentJobsResult = tx.exec(
			"SELECT "
			"    jp.id, "
			"    jp.job_title AS title, "
			"    COALESCE(cp.company_name, u.full_name) AS company, "
			"    jp.job_type AS type, "
			"    jp.status, "
			"    COALESCE(TO_CHAR(jp.application_deadline, 'YYYY-MM-DD'), '') AS deadline, "
			"    TO_CHAR(jp.created_at, 'YYYY-MM-DD') AS posted "
			"FROM job_postings jp "
			"INNER JOIN users u ON u.id = jp.employer_id "
			"LEFT JOIN company_profiles cp ON cp.user_id = u.id "
			"ORDER BY jp.created_at DESC "
			"LIMIT 5");

		for (const auto& row : recentJobsResult)
		{
			data.recentJobs.push_back(mapJobCard(pqxx::row(row)));
		}

		return data;
	}
	catch (const std::exception& e)
	{
		std::cout << "Get admin dashboard failed: " << e.what() << std::endl;
		return {};
	}
}

std::vector<S_JobCard> Database::getAdminJobs(const std::string& status)
{
	std::lock_guard<std::recursive_mutex> lock(m_dbMutex);
	std::vector<S_JobCard> jobs;
	if (!isConnected()) return jobs;

	try
	{
		pqxx::read_transaction tx(*m_connection);
		const auto result = tx.exec_params(
			"SELECT "
			"    jp.id, "
			"    jp.job_title AS title, "
			"    COALESCE(cp.company_name, u.full_name) AS company, "
			"    jp.job_type AS type, "
			"    jp.status, "
			"    COALESCE(TO_CHAR(jp.application_deadline, 'YYYY-MM-DD'), '') AS deadline, "
			"    TO_CHAR(jp.created_at, 'YYYY-MM-DD') AS posted "
			"FROM job_postings jp "
			"INNER JOIN users u ON u.id = jp.employer_id "
			"LEFT JOIN company_profiles cp ON cp.user_id = u.id "
			"WHERE ($1 = '' OR jp.status = $1) "
			"ORDER BY jp.created_at DESC",
			status);

		for (const auto& row : result)
		{
			jobs.push_back(mapJobCard(pqxx::row(row)));
		}

		return jobs;
	}
	catch (const std::exception& e)
	{
		std::cout << "Get admin jobs failed: " << e.what() << std::endl;
		return {};
	}
}

bool Database::saveCompanyProfile(const std::string& email, const S_CompanyProfile& input)
{
	std::lock_guard<std::recursive_mutex> lock(m_dbMutex);
	if (!isConnected()) return false;
	if (input.companyName.empty()) return false;

	try
	{
		pqxx::work tx(*m_connection);

		const pqxx::result result = tx.exec_params(
			"INSERT INTO company_profiles ("
			"    user_id, company_name, company_email, company_phone, industry, company_size, "
			"    company_location, company_description, website_url"
			") "
			"SELECT "
			"    u.id, $2, $3, $4, $5, $6, $7, $8, $9 "
			"FROM users u "
			"WHERE u.email = $1 AND u.role = 'employer' "
			"ON CONFLICT (user_id) DO UPDATE SET "
			"    company_name = EXCLUDED.company_name, "
			"    company_email = EXCLUDED.company_email, "
			"    company_phone = EXCLUDED.company_phone, "
			"    industry = EXCLUDED.industry, "
			"    company_size = EXCLUDED.company_size, "
			"    company_location = EXCLUDED.company_location, "
			"    company_description = EXCLUDED.company_description, "
			"    website_url = EXCLUDED.website_url, "
			"    updated_at = CURRENT_TIMESTAMP "
			"RETURNING user_id",
			email,
			input.companyName,
			input.companyEmail,
			input.companyPhone,
			input.industry,
			input.companySize,
			input.companyLocation,
			input.companyDescription,
			input.websiteUrl);

		if (result.empty())
		{
			return false;
		}

		tx.commit();
		return true;
	}
	catch (const std::exception& e)
	{
		std::cout << "Save company profile failed: " << e.what() << std::endl;
		return false;
	}
}

std::optional<S_CompanyProfile> Database::getCompanyProfile(const std::string& email)
{
	std::lock_guard<std::recursive_mutex> lock(m_dbMutex);
	if (!isConnected()) return std::nullopt;

	try
	{
		pqxx::read_transaction tx(*m_connection);
		const pqxx::result result = tx.exec_params(
			"SELECT "
			"    cp.company_name, "
			"    cp.company_email, "
			"    cp.company_phone, "
			"    cp.industry, "
			"    cp.company_size, "
			"    cp.company_location, "
			"    cp.company_description, "
			"    cp.website_url "
			"FROM company_profiles cp "
			"INNER JOIN users u ON u.id = cp.user_id "
			"WHERE u.email = $1 AND u.role = 'employer' "
			"LIMIT 1",
			email);

		if (result.empty())
		{
			return std::nullopt;
		}

		const auto& row = result[0];
		S_CompanyProfile profile;
		profile.companyName = row["company_name"].as<std::string>();
		if (!row["company_email"].is_null()) profile.companyEmail = row["company_email"].as<std::string>();
		if (!row["company_phone"].is_null()) profile.companyPhone = row["company_phone"].as<std::string>();
		if (!row["industry"].is_null()) profile.industry = row["industry"].as<std::string>();
		if (!row["company_size"].is_null()) profile.companySize = row["company_size"].as<std::string>();
		if (!row["company_location"].is_null()) profile.companyLocation = row["company_location"].as<std::string>();
		if (!row["company_description"].is_null()) profile.companyDescription = row["company_description"].as<std::string>();
		if (!row["website_url"].is_null()) profile.websiteUrl = row["website_url"].as<std::string>();

		return profile;
	}
	catch (const std::exception& e)
	{
		std::cout << "Get company profile failed: " << e.what() << std::endl;
		return std::nullopt;
	}
}

std::optional<S_JobCard> Database::getAdminJobDetails(long long jobId)
{
	std::lock_guard<std::recursive_mutex> lock(m_dbMutex);
	if (!isConnected()) return std::nullopt;

	try
	{
		pqxx::read_transaction tx(*m_connection);
		const auto result = tx.exec_params(
			"SELECT "
			"    jp.id, "
			"    jp.job_title AS title, "
			"    COALESCE(cp.company_name, u.full_name) AS company, "
			"    COALESCE(cp.company_description, '') AS company_description, "
			"    COALESCE(cp.website_url, '') AS company_website_url, "
			"    jp.job_type AS type, "
			"    COALESCE(jp.career_level, '') AS career_level, "
			"    jp.status, "
			"    COALESCE(TO_CHAR(jp.application_deadline, 'YYYY-MM-DD'), '') AS deadline, "
			"    TO_CHAR(jp.created_at, 'YYYY-MM-DD') AS posted, "
			"    COALESCE(jp.job_location, '') AS location, "
			"    COALESCE(jp.work_mode, '') AS work_mode, "
			"    COALESCE(jp.required_skills, '') AS required_skills, "
			"    COALESCE(jp.required_education, '') AS required_education, "
			"    COALESCE(jp.required_experience, 0) AS required_experience, "
			"    COALESCE(jp.job_description, '') AS job_description, "
			"    COALESCE(jp.salary_min::text, '') AS salary_min, "
			"    COALESCE(jp.salary_max::text, '') AS salary_max, "
			"    CASE "
			"        WHEN jp.salary_min IS NULL AND jp.salary_max IS NULL THEN '' "
			"        WHEN jp.salary_min IS NOT NULL AND jp.salary_max IS NOT NULL THEN jp.salary_min::text || ' - ' || jp.salary_max::text "
			"        ELSE COALESCE(jp.salary_min::text, jp.salary_max::text) "
			"    END AS salary_range "
			"FROM job_postings jp "
			"INNER JOIN users u ON u.id = jp.employer_id "
			"LEFT JOIN company_profiles cp ON cp.user_id = u.id "
			"WHERE jp.id = $1 "
			"LIMIT 1",
			jobId);

		if (result.empty())
		{
			return std::nullopt;
		}

		return mapJobCard(pqxx::row(result[0]));
	}
	catch (const std::exception& e)
	{
		std::cout << "Get admin job details failed: " << e.what() << std::endl;
		return std::nullopt;
	}
}

std::optional<S_CandidateCard> Database::getAdminCandidateDetails(long long candidateId)
{
	return getEmployerCandidateDetails(candidateId);
}

std::optional<S_AdminEmployerDetail> Database::getAdminEmployerDetails(long long employerId)
{
	std::lock_guard<std::recursive_mutex> lock(m_dbMutex);
	if (!isConnected()) return std::nullopt;

	try
	{
		pqxx::read_transaction tx(*m_connection);
		const auto result = tx.exec_params(
			"SELECT "
			"    u.id, "
			"    u.full_name, "
			"    u.email, "
			"    u.membership_status, "
			"    COALESCE(TO_CHAR(u.created_at, 'YYYY-MM-DD'), '') AS created_at, "
			"    COALESCE(cp.company_name, '') AS company_name, "
			"    COALESCE(cp.company_email, '') AS company_email, "
			"    COALESCE(cp.company_phone, '') AS company_phone, "
			"    COALESCE(cp.industry, '') AS industry, "
			"    COALESCE(cp.company_size, '') AS company_size, "
			"    COALESCE(cp.company_location, '') AS company_location, "
			"    COALESCE(cp.company_description, '') AS company_description, "
			"    COALESCE(cp.website_url, '') AS website_url, "
			"    COUNT(jp.id)::int AS total_jobs "
			"FROM users u "
			"LEFT JOIN company_profiles cp ON cp.user_id = u.id "
			"LEFT JOIN job_postings jp ON jp.employer_id = u.id "
			"WHERE u.role = 'employer' AND u.id = $1 "
			"GROUP BY u.id, u.full_name, u.email, u.membership_status, u.created_at, cp.company_name, cp.company_email, cp.company_phone, cp.industry, cp.company_size, cp.company_location, cp.company_description, cp.website_url "
			"LIMIT 1",
			employerId);

		if (result.empty())
		{
			return std::nullopt;
		}

		const auto& row = result[0];
		S_AdminEmployerDetail item;
		item.id = row["id"].as<long long>();
		item.fullName = row["full_name"].as<std::string>();
		item.email = row["email"].as<std::string>();
		item.membershipStatus = row["membership_status"].as<std::string>();
		item.createdAt = row["created_at"].as<std::string>();
		item.companyName = row["company_name"].as<std::string>();
		item.companyEmail = row["company_email"].as<std::string>();
		item.companyPhone = row["company_phone"].as<std::string>();
		item.industry = row["industry"].as<std::string>();
		item.companySize = row["company_size"].as<std::string>();
		item.companyLocation = row["company_location"].as<std::string>();
		item.companyDescription = row["company_description"].as<std::string>();
		item.websiteUrl = row["website_url"].as<std::string>();
		item.totalJobs = row["total_jobs"].as<int>();
		return item;
	}
	catch (const std::exception& e)
	{
		std::cout << "Get admin employer details failed: " << e.what() << std::endl;
		return std::nullopt;
	}
}

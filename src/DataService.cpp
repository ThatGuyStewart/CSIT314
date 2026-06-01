#include "DataService.h"
#include "ScoringLogic.h"
#include <algorithm>
#include <thread>
#include <unordered_set>

namespace
{
	S_CandidateProfile toCandidateProfile(const S_CandidateCard& candidate)
	{
		S_CandidateProfile profile;
		profile.contactInfo = candidate.contactInfo;
		profile.education = candidate.education;
		profile.major = candidate.major;
		profile.yearsExperience = candidate.yearsExperience;
		profile.workExperience = candidate.workExperience;
		profile.skills = candidate.skills;
		profile.preferredWorkMode = candidate.preferredWorkMode;
		profile.preferredLocation = candidate.preferredLocation;
		profile.expectedSalary = candidate.expectedSalary;
		profile.employmentType = candidate.employmentType;
		profile.summary = candidate.summary;
		profile.portfolioUrl = candidate.portfolioUrl;
		return profile;
	}

	S_CandidateCard toCandidateCard(const S_EmployerCandidateRecommendation& recommendation)
	{
		S_CandidateCard candidate;
		candidate.id = recommendation.candidateId;
		candidate.fullName = recommendation.fullName;
		candidate.major = recommendation.major;
		candidate.skills = recommendation.skills;
		candidate.preferredLocation = recommendation.preferredLocation;
		candidate.preferredWorkMode = recommendation.preferredWorkMode;
		candidate.experienceText = recommendation.experienceText;
		candidate.summary = recommendation.summary;
		candidate.matchedJobId = recommendation.jobId;
		candidate.matchedJobTitle = recommendation.jobTitle;
		candidate.matchScore = recommendation.matchScore;
		candidate.matchReasons = recommendation.matchReasons;
		return candidate;
	}
}

DataService::DataService(Database& database)
	: m_database(database)
{
}

DataService::~DataService()
{
}

bool DataService::isConnected() const
{
	return m_database.isConnected();
}


bool DataService::validateAccount(const std::string& email, const std::string& password)
{
	return m_database.validateAccount(email, password);
}

bool DataService::accountExists(const std::string& email)
{
	return m_database.accountExists(email);
}

bool DataService::createAccount(const std::string& fullName, const std::string& email, const std::string& password, const std::string& role)
{
	return m_database.createAccount(fullName, email, password, role);
}

bool DataService::isAdminAccount(const std::string& email)
{
	return m_database.isAdminAccount(email);
}

std::optional<std::string> DataService::getAccountRole(const std::string& email)
{
	return m_database.getAccountRole(email);
}

bool DataService::updateMembershipStatus(const std::string& email, const std::string& membershipStatus)
{
	return m_database.updateMembershipStatus(email, membershipStatus);
}

bool DataService::saveCandidateProfile(const std::string& email, const S_CandidateProfile& input)
{
	return m_database.saveCandidateProfile(email, input);
}

std::optional<S_CandidateProfile> DataService::getCandidateProfile(const std::string& email)
{
	return m_database.getCandidateProfile(email);
}

bool DataService::createJobPosting(const std::string& email, const S_JobListing& input)
{
	return m_database.createJobPosting(email, input);
}

std::vector<S_UserSummary> DataService::getAdminUsers(const std::string& search, const std::string& role)
{
	auto users = m_database.getAdminUsers(role);

	if (search.empty())
	{
		return users;
	}

	users.erase(std::remove_if(users.begin(), users.end(), [&](const S_UserSummary& user)
		{
			return ScoringLogic::GetUserSearchScore(user, search) <= 0.0f;
		}), users.end());

	if (users.size() < 2)
	{
		return users;
	}

	std::stable_sort(users.begin(), users.end(), [&](const S_UserSummary& left, const S_UserSummary& right)
		{
			return ScoringLogic::GetUserSearchScore(left, search) > ScoringLogic::GetUserSearchScore(right, search);
		});

	return users;
}

S_CandidateDashboardData DataService::getCandidateDashboard(const std::string& email)
{
	auto data = m_database.getCandidateDashboard(email);
	data.topRecommendations = getRecommendedJobs(email);
	data.recommendedCount = static_cast<int>(data.topRecommendations.size());
	if (data.topRecommendations.size() > 3)
	{
		data.topRecommendations.resize(3);
	}

	return data;
}

std::vector<S_JobCard> DataService::getCandidateJobs(const std::string& keyword, const std::string& location, const std::string& workMode, const std::string& jobType, const std::string& careerLevel, const std::string& salaryMin, const std::string& salaryMax)
{
	auto jobs = m_database.getFilteredCandidateJobs(location, workMode, jobType, careerLevel, salaryMin, salaryMax);

	if (keyword.empty())
	{
		return jobs;
	}

	jobs.erase(std::remove_if(jobs.begin(), jobs.end(), [&](const S_JobCard& job)
		{
			return !ScoringLogic::HasFuzzyKeywordMatch(job, keyword);
		}), jobs.end());

	if (jobs.size() < 2)
	{
		return jobs;
	}

	std::stable_sort(jobs.begin(), jobs.end(), [&](const S_JobCard& left, const S_JobCard& right)
		{
			return ScoringLogic::GetJobKeywordScore(left, keyword) > ScoringLogic::GetJobKeywordScore(right, keyword);
		});

	return jobs;
}

std::optional<S_JobCard> DataService::getCandidateJobDetails(const std::string& email, long long jobId)
{
	auto job = m_database.getCandidateJobDetails(jobId);
	if (!job.has_value())
	{
		return std::nullopt;
	}

	job->isApplied = m_database.hasCandidateAppliedToJob(email, jobId);

	const auto candidateProfile = m_database.getCandidateProfile(email);
	if (!candidateProfile.has_value())
	{
		job->matchScore = 0;
		job->matchReasons = { "Complete your profile to see personalized match insights." };
		return job;
	}

	const auto evaluation = ScoringLogic::EvaluateCandidateJobForCandidate(candidateProfile.value(), job.value());
	job->matchScore = evaluation.matchScore;
	job->matchReasons = evaluation.matchReasons;
	return job;
}

bool DataService::applyToCandidateJob(const std::string& email, long long jobId)
{
	return m_database.createCandidateApplication(email, jobId);
}

std::vector<S_JobCard> DataService::getRecommendedJobs(const std::string& email)
{
	auto jobs = m_database.getCandidateJobs();

	const auto candidateProfile = m_database.getCandidateProfile(email);
	if (!candidateProfile.has_value())
	{
			if (jobs.size() < 2)
			{
				return jobs;
			}
		return jobs;
	}

	for (auto& job : jobs)
	{
		const auto evaluation = ScoringLogic::EvaluateCandidateJobScoreOnly(candidateProfile.value(), job);
		job.matchScore = evaluation.matchScore;
		job.matchReasons.clear();
	}

		jobs.erase(std::remove_if(jobs.begin(), jobs.end(), [&](const S_JobCard& job)
			{
				return job.matchScore <= 0;
			}), jobs.end());

		if (jobs.size() < 2)
		{
			return jobs;
		}

	std::stable_sort(jobs.begin(), jobs.end(), [&](const S_JobCard& left, const S_JobCard& right)
		{
			return left.matchScore > right.matchScore;
		});

	return jobs;
}

S_EmployerDashboardData DataService::getEmployerDashboard(const std::string& email)
{
	auto data = m_database.getEmployerDashboard(email);
	const auto recommendations = getEmployerRecommendations(email, 0, 3);

	data.topCandidates.clear();
	data.topCandidates.reserve(std::min<size_t>(3, recommendations.size()));

	for (const auto& recommendation : recommendations)
	{
		data.topCandidates.push_back(toCandidateCard(recommendation));
		if (data.topCandidates.size() >= 3)
		{
			break;
		}
	}

	return data;
}

std::vector<S_CandidateCard> DataService::getEmployerCandidates(const std::string& keyword, const std::string& skills, const std::string& education, const std::string& yearsExperience, const std::string& preferredWorkMode, const std::string& preferredLocation)
{
	auto candidates = m_database.getFilteredEmployerCandidates(education, yearsExperience, preferredWorkMode, preferredLocation);

	if (!keyword.empty() || !skills.empty())
	{
		candidates.erase(std::remove_if(candidates.begin(), candidates.end(), [&](const S_CandidateCard& candidate)
			{
				return !ScoringLogic::HasFuzzyCandidateMatch(candidate, keyword, skills);
			}), candidates.end());
	}

	if ((keyword.empty() && skills.empty()) || candidates.size() < 2)
	{
		return candidates;
	}

	std::stable_sort(candidates.begin(), candidates.end(), [&](const S_CandidateCard& left, const S_CandidateCard& right)
		{
			return ScoringLogic::GetCandidateSearchScore(left, keyword, skills) > ScoringLogic::GetCandidateSearchScore(right, keyword, skills);
		});

	return candidates;
}

std::optional<S_CandidateCard> DataService::getEmployerCandidateDetails(const std::string& email, long long candidateId, long long jobId)
{
	auto candidate = m_database.getEmployerCandidateDetails(candidateId);
	if (!candidate.has_value())
	{
		return std::nullopt;
	}

	auto jobs = m_database.getEmployerJobs(email);
	if (jobId > 0)
	{
		jobs.erase(std::remove_if(jobs.begin(), jobs.end(), [&](const S_JobCard& job)
			{
				return job.id != jobId;
			}), jobs.end());
	}

	if (jobs.empty())
	{
		candidate->matchScore = 0;
		candidate->matchReasons = { jobId > 0 ? "The selected job could not be found for match comparison." : "Post a job to see how this candidate matches against your openings." };
		candidate->matchedJobId = 0;
		candidate->matchedJobTitle.clear();
		return candidate;
	}

	const auto candidateProfile = toCandidateProfile(candidate.value());
	ScoringLogic::MatchEvaluation bestEvaluation;
	const S_JobCard* bestJob = nullptr;
	for (const auto& job : jobs)
	{
		const auto evaluation = ScoringLogic::EvaluateCandidateJobForEmployer(candidateProfile, job);
		if (!bestJob || evaluation.rawScore > bestEvaluation.rawScore)
		{
			bestJob = &job;
			bestEvaluation = evaluation;
		}
	}

	if (!bestJob)
	{
		candidate->matchScore = 0;
		candidate->matchReasons = { "No suitable employer jobs were available for comparison." };
		candidate->matchedJobId = 0;
		candidate->matchedJobTitle.clear();
		return candidate;
	}

	candidate->matchedJobId = bestJob->id;
	candidate->matchedJobTitle = bestJob->title;
	candidate->matchScore = bestEvaluation.matchScore;
	candidate->matchReasons = bestEvaluation.matchReasons;
	return candidate;
}

std::vector<S_EmployerCandidateRecommendation> DataService::getEmployerRecommendations(const std::string& email, long long jobId, size_t maxResults)
{
	const auto dashboard = m_database.getEmployerDashboard(email);
	auto jobs = m_database.getEmployerJobs(email);
	if (jobId > 0)
	{
		jobs.erase(std::remove_if(jobs.begin(), jobs.end(), [&](const S_JobCard& job)
			{
				return job.id != jobId;
			}), jobs.end());
	}

	if (jobs.empty())
	{
		return {};
	}

	auto candidates = m_database.getEmployerCandidates();
	std::vector<S_EmployerCandidateRecommendation> recommendations;
	recommendations.reserve(candidates.size() * jobs.size());
	const bool includeReasons = jobId > 0;

	for (const auto& candidate : candidates)
	{
		const auto candidateProfile = toCandidateProfile(candidate);
		for (const auto& job : jobs)
		{
				const auto scoreOnly = ScoringLogic::EvaluateCandidateJobScoreOnly(candidateProfile, job);
				std::vector<std::string> matchReasons;
				if (includeReasons)
				{
					matchReasons = ScoringLogic::BuildEmployerMatchReasons(candidateProfile, job);
				}
				const int matchScore = scoreOnly.matchScore;
				if (matchScore <= 0)
				{
					continue;
				}

			S_EmployerCandidateRecommendation recommendation;
			recommendation.candidateId = candidate.id;
			recommendation.fullName = candidate.fullName;
			recommendation.major = candidate.major;
			recommendation.skills = candidate.skills;
			recommendation.preferredLocation = candidate.preferredLocation;
			recommendation.preferredWorkMode = candidate.preferredWorkMode;
			recommendation.experienceText = candidate.experienceText;
			recommendation.summary = candidate.summary;
			recommendation.jobId = job.id;
			recommendation.jobTitle = job.title;
			recommendation.rawScore = scoreOnly.rawScore;
				recommendation.matchScore = matchScore;
			recommendation.matchReasons = std::move(matchReasons);
			recommendations.push_back(std::move(recommendation));
		}
	}

	std::stable_sort(recommendations.begin(), recommendations.end(), [](const S_EmployerCandidateRecommendation& left, const S_EmployerCandidateRecommendation& right)
		{
			if (left.matchScore != right.matchScore)
			{
				return left.matchScore > right.matchScore;
			}

			if (left.rawScore != right.rawScore)
			{
				return left.rawScore > right.rawScore;
			}

			return left.candidateId < right.candidateId;
		});

	if (jobId <= 0)
	{
		std::vector<S_EmployerCandidateRecommendation> bestRecommendations;
		bestRecommendations.reserve(recommendations.size());
		std::unordered_map<long long, std::vector<S_JobCard>> matchedJobsByCandidate;

		for (const auto& recommendation : recommendations)
		{
			S_JobCard matchedJob;
			matchedJob.id = recommendation.jobId;
			matchedJob.title = recommendation.jobTitle;
			matchedJob.matchScore = recommendation.matchScore;
			matchedJobsByCandidate[recommendation.candidateId].push_back(std::move(matchedJob));
		}

		std::unordered_set<long long> seenCandidateIds;

		for (const auto& recommendation : recommendations)
		{
			if (seenCandidateIds.insert(recommendation.candidateId).second)
			{
				auto bestRecommendation = recommendation;
				auto matchedJobsIt = matchedJobsByCandidate.find(recommendation.candidateId);
				if (matchedJobsIt != matchedJobsByCandidate.end())
				{
					bestRecommendation.matchedJobs = std::move(matchedJobsIt->second);
				}

				bestRecommendations.push_back(std::move(bestRecommendation));
			}
		}

		recommendations = std::move(bestRecommendations);
	}

	if (maxResults > 0 && recommendations.size() > maxResults)
	{
		recommendations.resize(maxResults);
	}

	if (dashboard.membershipStatus == "free" && recommendations.size() > 10)
	{
		recommendations.resize(10);
	}

	return recommendations;
}

std::optional<S_JobCard> DataService::getEmployerJobDetails(const std::string& email, long long jobId)
{
	return m_database.getEmployerJobDetails(email, jobId);
}

bool DataService::updateEmployerJob(const std::string& email, long long jobId, const S_JobListing& input)
{
	return m_database.updateEmployerJob(email, jobId, input);
}

std::vector<S_JobCard> DataService::getEmployerJobs(const std::string& email)
{
	return m_database.getEmployerJobs(email);
}

S_AdminDashboardData DataService::getAdminDashboard()
{
	return m_database.getAdminDashboard();
}

std::vector<S_JobCard> DataService::getAdminJobs(const std::string& search, const std::string& status)
{
	auto jobs = m_database.getAdminJobs(status);

	if (search.empty())
	{
		return jobs;
	}

	jobs.erase(std::remove_if(jobs.begin(), jobs.end(), [&](const S_JobCard& job)
		{
			return !ScoringLogic::HasFuzzyKeywordMatch(job, search);
		}), jobs.end());

	if (jobs.size() < 2)
	{
		return jobs;
	}

	std::stable_sort(jobs.begin(), jobs.end(), [&](const S_JobCard& left, const S_JobCard& right)
		{
			return ScoringLogic::GetJobKeywordScore(left, search) > ScoringLogic::GetJobKeywordScore(right, search);
		});

	return jobs;
}

std::optional<S_CandidateCard> DataService::getAdminCandidateDetails(long long candidateId)
{
	return m_database.getAdminCandidateDetails(candidateId);
}

std::optional<S_AdminEmployerDetail> DataService::getAdminEmployerDetails(long long employerId)
{
	return m_database.getAdminEmployerDetails(employerId);
}

std::optional<S_JobCard> DataService::getAdminJobDetails(long long jobId)
{
	return m_database.getAdminJobDetails(jobId);
}

bool DataService::saveCompanyProfile(const std::string& email, const S_CompanyProfile& input)
{
	return m_database.saveCompanyProfile(email, input);
}

std::optional<S_CompanyProfile> DataService::getCompanyProfile(const std::string& email)
{
	return m_database.getCompanyProfile(email);
}

void DataService::stop()
{
}

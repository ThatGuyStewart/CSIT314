#pragma once

#include "Structs.h"
#include <string>
#include <vector>

class ScoringLogic
{
public:
	struct MatchEvaluation
	{
		float rawScore = 0.0f;
		int matchScore = 0;
		std::vector<std::string> matchReasons;
	};

	struct ScoreOnlyEvaluation
	{
		float rawScore = 0.0f;
		int matchScore = 0;
	};

	static float GetJobKeywordScore(const S_JobCard& job, const std::string& keyword);
	static bool HasFuzzyKeywordMatch(const S_JobCard& job, const std::string& keyword);
	static float GetUserSearchScore(const S_UserSummary& user, const std::string& query);
	static float GetCandidateSearchScore(const S_CandidateCard& candidate, const std::string& keyword, const std::string& skills);
	static bool HasFuzzyCandidateMatch(const S_CandidateCard& candidate, const std::string& keyword, const std::string& skills);
	static float GetSkillMatchScore(const S_CandidateProfile& candidate, const S_JobCard& job);
	static float GetFieldAlignmentScore(const S_CandidateProfile& candidate, const S_JobCard& job);
	static float GetMajorRelevanceScore(const S_CandidateProfile& candidate, const S_JobCard& job);
	static float GetRecommendedJobScore(const S_CandidateProfile& candidate, const S_JobCard& job);
	static int ToMatchPercentage(float score);
	static ScoreOnlyEvaluation EvaluateCandidateJobScoreOnly(const S_CandidateProfile& candidate, const S_JobCard& job);
	static MatchEvaluation EvaluateCandidateJobForCandidate(const S_CandidateProfile& candidate, const S_JobCard& job);
	static MatchEvaluation EvaluateCandidateJobForEmployer(const S_CandidateProfile& candidate, const S_JobCard& job);
	static std::vector<std::string> BuildCandidateMatchReasons(const S_CandidateProfile& candidate, const S_JobCard& job);
	static std::vector<std::string> BuildEmployerMatchReasons(const S_CandidateProfile& candidate, const S_JobCard& job);
};
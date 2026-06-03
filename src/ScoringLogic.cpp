#include "ScoringLogic.h"
#include "zadeh.h"
#include <algorithm>
#include <cmath>
#include <cctype>
#include <limits>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace
{
	// Enums to represent the relationship between candidate and job domains, role families, and the audience for match reasons. These enums help categorize the strength of matches and the relevance of certain criteria in the scoring process.
	enum class DomainRelationship { None, Related, Adjacent };
	enum class RoleFamilyRelationship { None, Related, Adjacent };
	enum class MatchReasonAudience { Candidate, Employer };
	enum class EducationLevel
	{
		Any,
		HighSchool,
		Bachelors,
		Masters,
		PhD,
		Other
	};

	// Map to hold domain signals for different categories (e.g., job title, skills) during the scoring process, allowing for detailed analysis of which aspects of a candidate's profile contributed to their match score
	using DomainSignalMap = std::unordered_map<std::string, DomainSignal>;

	// Structure to hold expanded search queries for job searches, including separate lists for phrase queries (which contain multiple words) and token queries (which are single words). This allows for more flexible and comprehensive matching of job search keywords against job titles and descriptions.
	struct ExpandedJobSearchQueries
	{
		std::vector<std::string> phraseQueries;
		std::vector<std::string> tokenQueries;
	};

	// Structure to hold information about how well a specific domain (e.g., job title, skills) matches the search criteria, including the count of matched terms and the specific terms that were matched
	struct DomainSignal
	{
		int matchCount = 0;
		std::vector<std::string> matchedTerms;
	};

	// Normalize education text by converting to lowercase and removing non-alphanumeric characters, while preserving spaces for better matching
	std::string normalizeEducationText(const std::string& value)
	{
		std::string normalized;
		normalized.reserve(value.size());

		for (unsigned char ch : value)
		{
			if (std::isalnum(ch))
			{
				normalized.push_back(static_cast<char>(std::tolower(ch)));
			}
			else if (std::isspace(ch))
			{
				normalized.push_back(' ');
			}
		}

		return normalized;
	}

	// Convert normalized education text to an EducationLevel enum value based on common keywords and phrases
	EducationLevel educationLevelFromString(const std::string& value)
	{
		const std::string normalized = normalizeEducationText(value);

		if (normalized.empty() || normalized.find("any") != std::string::npos)
		{
			return EducationLevel::Any;
		}
		if (normalized.find("high school") != std::string::npos)
		{
			return EducationLevel::HighSchool;
		}
		if (normalized.find("bachelor") != std::string::npos)
		{
			return EducationLevel::Bachelors;
		}
		if (normalized.find("master") != std::string::npos)
		{
			return EducationLevel::Masters;
		}
		if (normalized.find("phd") != std::string::npos)
		{
			return EducationLevel::PhD;
		}

		return EducationLevel::Other;
	}

	// Assign a rank to each education level for comparison purposes, with higher ranks indicating higher levels of education
	int educationLevelRank(EducationLevel level)
	{
		switch (level)
		{
		case EducationLevel::HighSchool:
			return 1;
		case EducationLevel::Bachelors:
			return 2;
		case EducationLevel::Masters:
			return 3;
		case EducationLevel::PhD:
			return 4;
		default:
			return 0;
		}
	}

	// Normalize search text by converting to lowercase, removing non-alphanumeric characters (except for '+' and '#'), and collapsing multiple spaces into a single space. This helps improve matching accuracy by standardizing the format of the text.
	std::string normalizeSearchText(const std::string& value)
	{
		std::string normalized;
		normalized.reserve(value.size());

		bool lastWasSpace = false;
		for (unsigned char ch : value)
		{
			if (std::isalnum(ch) || ch == '+' || ch == '#')
			{
				normalized.push_back(static_cast<char>(std::tolower(ch)));
				lastWasSpace = false;
			}
			else if (!normalized.empty() && !lastWasSpace)
			{
				normalized.push_back(' ');
				lastWasSpace = true;
			}
		}

		if (!normalized.empty() && normalized.back() == ' ')
		{
			normalized.pop_back();
		}

		return normalized;
	}
	
	// Public wrapper for normalizing text, which currently uses the same normalization logic as normalizeSearchText but can be modified in the future if needed without affecting the internal implementation
	std::string normalizeText(const std::string& value)
	{
		return normalizeSearchText(value);
	}

	bool containsKeywordTerm(const std::string& text, const std::string& keyword)
	{
		const std::string normalizedText = normalizeSearchText(text);
		const std::string normalizedKeyword = normalizeSearchText(keyword);

		if (normalizedText.empty() || normalizedKeyword.empty())
		{
			return false;
		}

		if (normalizedText == normalizedKeyword)
		{
			return true;
		}

		const std::string paddedText = " " + normalizedText + " ";
		const std::string paddedKeyword = " " + normalizedKeyword + " ";
		return paddedText.find(paddedKeyword) != std::string::npos;
	}

	// Calculate a fuzzy matching score between a candidate string and a query string using the Zadeh library. The score is based on the similarity of the two strings, with higher scores indicating a closer match. If either string is empty, a score of 0 is returned.
	float getFuzzyScore(const std::string& candidate, const std::string& query)
	{
		if (candidate.empty() || query.empty())
		{
			return 0.0f;
		}

		const zadeh::Options options(query, 0u, false, false);
		return zadeh::scorer_score(candidate, query, options);
	}

	// Append a normalized version of the value to the list of values if it is not already present. This helps ensure that the list of values contains unique, normalized entries for better matching and scoring.
	void appendUniqueNormalized(std::vector<std::string>& values, const std::string& value)
	{
		const std::string normalized = normalizeSearchText(value);
		if (!normalized.empty() && std::find(values.begin(), values.end(), normalized) == values.end())
		{
			values.push_back(normalized);
		}
	}

	// Expand a job search keyword into multiple related queries, including both phrase queries (which contain multiple words) and token queries (which are single words). This allows for more comprehensive matching of job search keywords against job titles and descriptions, improving the relevance of search results.
	ExpandedJobSearchQueries expandJobSearchQueries(const std::string& keyword)
	{
		ExpandedJobSearchQueries queries;
		const auto appendQuery = [&](const std::string& value)
		{
			const std::string normalized = normalizeSearchText(value);
			if (normalized.empty())
			{
				return;
			}

			if (normalized.find(' ') != std::string::npos)
			{
				appendUniqueNormalized(queries.phraseQueries, normalized);
			}
			else
			{
				appendUniqueNormalized(queries.tokenQueries, normalized);
			}
		};

		appendQuery(keyword);

		const std::string normalizedKeyword = normalizeSearchText(keyword);
		const auto addAliases = [&](std::initializer_list<const char*> triggers, std::initializer_list<const char*> aliases)
		{
			for (const auto* trigger : triggers)
			{
				const std::string normalizedTrigger = normalizeSearchText(trigger);
				if (!normalizedTrigger.empty() && (normalizedKeyword == normalizedTrigger || getFuzzyScore(normalizedTrigger, normalizedKeyword) > 0.72f || getFuzzyScore(normalizedKeyword, normalizedTrigger) > 0.72f))
				{
					for (const auto* alias : aliases)
					{
						appendQuery(alias);
					}
					return;
				}
			}
		};

		addAliases({ "software engineer", "software engineering", "software developer", "programmer", "coder", "software enginer", "sofware enginer" },
			{ "software engineer", "software developer", "backend developer", "frontend developer", "full stack developer", "backend", "frontend", "full stack", "programmer", "coder", "software engineering" });
		addAliases({ "frontend", "front end", "web developer", "ui developer" },
			{ "frontend developer", "web developer", "react developer", "ui developer" });
		addAliases({ "backend", "back end", "api developer", "services developer" },
			{ "backend developer", "backend engineer", "api developer", "services developer" });
		addAliases({ "full stack", "fullstack" },
			{ "full stack developer", "frontend developer", "backend developer" });
		addAliases({ "medical", "medicine", "clinical", "biomedical", "health", "healthcare", "public health" },
			{ "medical", "clinical", "biomedical", "healthcare", "health", "public health", "laboratory", "diagnostics" });
		addAliases({ "manager", "coordinator", "supervisor" },
			{ "manager", "coordinator", "supervisor" });

		return queries;
	}

	// Split the normalized search text into individual terms (tokens) based on spaces, allowing for more granular matching of search queries against candidate profiles and job descriptions. This helps improve the relevance of search results by considering each term separately.
	std::vector<std::string> splitSearchTerms(const std::string& value)
	{
		std::vector<std::string> terms;
		std::string current;

		for (char ch : normalizeSearchText(value))
		{
			if (ch != ' ')
			{
				current.push_back(ch);
			}
			else if (!current.empty())
			{
				terms.push_back(current);
				current.clear();
			}
		}

		if (!current.empty())
		{
			terms.push_back(current);
		}

		return terms;
	}

	// Check if the normalized search text contains the specified term as a whole word, rather than as part of another word. This helps improve the accuracy of search results by ensuring that matches are based on complete terms rather than partial matches.
	bool containsWholeSearchTerm(const std::string& text, const std::string& term)
	{
		for (const auto& token : splitSearchTerms(text))
		{
			if (token == term)
			{
				return true;
			}
		}

		return false;
	}

	// Calculate the edit distance (Levenshtein distance) between two strings, which is the minimum number of single-character edits (insertions, deletions, substitutions) required to change one string into the other. This is used for approximate matching of search terms against candidate profiles and job descriptions.
	size_t getEditDistance(const std::string& left, const std::string& right)
	{
		std::vector<size_t> previous(right.size() + 1);
		std::vector<size_t> current(right.size() + 1);

		for (size_t index = 0; index <= right.size(); ++index)
		{
			previous[index] = index;
		}

		for (size_t leftIndex = 0; leftIndex < left.size(); ++leftIndex)
		{
			current[0] = leftIndex + 1;
			for (size_t rightIndex = 0; rightIndex < right.size(); ++rightIndex)
			{
				const size_t insertionCost = current[rightIndex] + 1;
				const size_t deletionCost = previous[rightIndex + 1] + 1;
				const size_t substitutionCost = previous[rightIndex] + (left[leftIndex] == right[rightIndex] ? 0u : 1u);
				current[rightIndex + 1] = std::min({ insertionCost, deletionCost, substitutionCost });
			}

			previous.swap(current);
		}

		return previous[right.size()];
	}

	// Check if two tokens approximately match each other based on their edit distance, allowing for minor typos or variations in the search terms. The maximum allowed edit distance is determined based on the length of the tokens, with longer tokens allowing for more edits.
	bool isApproximateTokenMatch(const std::string& textToken, const std::string& queryToken)
	{
		if (textToken.empty() || queryToken.empty())
		{
			return false;
		}

		if (textToken == queryToken)
		{
			return true;
		}

		if (textToken.size() < 4 || queryToken.size() < 4)
		{
			return false;
		}

		const size_t maxLength = std::max(textToken.size(), queryToken.size());
		const size_t maxDistance = maxLength >= 8 ? 2u : 1u;
		const size_t lengthDifference = textToken.size() > queryToken.size()
			? textToken.size() - queryToken.size()
			: queryToken.size() - textToken.size();

		if (lengthDifference > maxDistance)
		{
			return false;
		}

		return getEditDistance(textToken, queryToken) <= maxDistance;
	}

	// Check if the normalized search text contains a term that approximately matches the specified term, allowing for minor typos or variations in the search terms. This helps improve the relevance of search results by considering approximate matches in addition to exact matches.
	bool containsApproximateWholeSearchTerm(const std::string& text, const std::string& term)
	{
		for (const auto& token : splitSearchTerms(text))
		{
			if (isApproximateTokenMatch(token, term))
			{
				return true;
			}
		}

		return false;
	}

	// Check if a candidate card has a keyword phrase match against the query, by checking if any of the candidate's relevant fields (full name, major, skills, summary, work experience, education) contain the query as a keyword term. This is used to determine if a candidate should be included in search results based on the presence of the search keyword in their profile.
	bool hasCandidateKeywordPhraseMatch(const S_CandidateCard& candidate, const std::string& query)
	{
		return containsKeywordTerm(candidate.fullName, query)
			|| containsKeywordTerm(candidate.major, query)
			|| containsKeywordTerm(candidate.skills, query)
			|| containsKeywordTerm(candidate.summary, query)
			|| containsKeywordTerm(candidate.workExperience, query)
			|| containsKeywordTerm(candidate.education, query);
	}

	// Check if a candidate card has a keyword token match against the query, by checking if any of the candidate's relevant fields (full name, major, skills, summary, work experience, education) contain the query as either a whole search term or an approximate whole search term. This allows for more flexible matching of search keywords against candidate profiles, improving the relevance of search results.
	bool hasCandidateKeywordTokenMatch(const S_CandidateCard& candidate, const std::string& query)
	{
		return containsWholeSearchTerm(candidate.fullName, query)
			|| containsWholeSearchTerm(candidate.major, query)
			|| containsWholeSearchTerm(candidate.skills, query)
			|| containsWholeSearchTerm(candidate.summary, query)
			|| containsWholeSearchTerm(candidate.workExperience, query)
			|| containsWholeSearchTerm(candidate.education, query)
			|| containsApproximateWholeSearchTerm(candidate.fullName, query)
			|| containsApproximateWholeSearchTerm(candidate.major, query)
			|| containsApproximateWholeSearchTerm(candidate.skills, query);
	}

	// Check if a candidate card has a skills phrase match against the query, by checking if any of the candidate's relevant fields (skills, major, summary) contain the query as a keyword term. This is used to determine if a candidate should be included in search results based on the presence of the search keyword in their skills or related fields.
	bool hasCandidateSkillsPhraseMatch(const S_CandidateCard& candidate, const std::string& query)
	{
		return containsKeywordTerm(candidate.skills, query)
			|| containsKeywordTerm(candidate.major, query)
			|| containsKeywordTerm(candidate.summary, query);
	}

	// Check if a candidate card has a skills token match against the query, by checking if any of the candidate's relevant fields (skills, major, summary) contain the query as either a whole search term or an approximate whole search term. This allows for more flexible matching of search keywords against candidate profiles, improving the relevance of search results based on skills and related fields.
	bool hasCandidateSkillsTokenMatch(const S_CandidateCard& candidate, const std::string& query)
	{
		return containsWholeSearchTerm(candidate.skills, query)
			|| containsWholeSearchTerm(candidate.major, query)
			|| containsWholeSearchTerm(candidate.summary, query)
			|| containsApproximateWholeSearchTerm(candidate.skills, query)
			|| containsApproximateWholeSearchTerm(candidate.major, query);
	}

	// Append normalized versions of the specified phrases to the list of tokens if they are present in the source text as keyword terms. This helps ensure that important multi-word phrases are included in the list of tokens for matching and scoring purposes, improving the relevance of search results.
	void appendPhraseTokens(std::vector<std::string>& tokens, const std::string& source, std::initializer_list<const char*> phrases)
	{
		for (const auto* phrase : phrases)
		{
			const std::string normalized = normalizeText(phrase);
			if (!normalized.empty() && containsKeywordTerm(source, normalized))
			{
				tokens.push_back(normalized);
			}
		}
	}

	// Split the normalized search text into individual tokens based on spaces, while also checking for the presence of important multi-word phrases and including them as tokens if they are found. This allows for more comprehensive matching of search queries against candidate profiles and job descriptions, improving the relevance of search results.
	std::vector<std::string> splitTerms(const std::string& value)
	{
		std::vector<std::string> tokens;
		std::string current;

		for (char ch : normalizeText(value))
		{
			if (ch != ' ')
			{
				current.push_back(ch);
			}
			else if (!current.empty())
			{
				if (current.size() > 1)
				{
					tokens.push_back(current);
				}
				current.clear();
			}
		}

		if (current.size() > 1)
		{
			tokens.push_back(current);
		}

		const std::string normalizedValue = normalizeText(value);

		appendPhraseTokens(tokens, normalizedValue, {
			"software engineering",
			"computer science",
			"information systems",
			"backend developer",
			"backend engineer",
			"cloud engineer",
			"full stack",
			"machine learning",
			"data science",
			"data analyst",
			"incident response",
			"network security",
			"user research",
			"design systems",
			"health systems",
			"lead generation",
			"supply chain",
			"supply chain management",
			"inventory control",
			"retail management",
			"retail operations",
			"video production",
			"film and media production",
			"audio mixing",
			"environmental science",
			"environmental monitoring",
			"regulatory reporting",
			"warehouse planning",
			"healthcare systems"
		});

		return tokens;
	}

	// Append a normalized version of the value to the list of tokens if it is not already present, ensuring that the list of tokens contains unique entries for better matching and scoring.
	void appendUniqueToken(std::vector<std::string>& tokens, const std::string& value)
	{
		if (!value.empty() && std::find(tokens.begin(), tokens.end(), value) == tokens.end())
		{
			tokens.push_back(value);
		}
	}

	// Check if the list of tokens contains a normalized version of the specified value, allowing for more flexible matching of search queries against candidate profiles and job descriptions. This helps improve the relevance of search results by considering normalized versions of search terms.
	bool containsNormalizedToken(const std::vector<std::string>& tokens, const char* value)
	{
		const std::string normalized = normalizeText(value);
		for (const auto& token : tokens)
		{
			if (token == normalized)
			{
				return true;
			}
		}

		return false;
	}

	// Check if any of the specified trigger phrases are present in the list of tokens, and if so, append the corresponding alias phrases to the list of tokens. This allows for the expansion of search queries based on the presence of certain key terms, improving the relevance of search results by considering related terms and phrases.
	void addSemanticGroup(std::vector<std::string>& tokens, std::initializer_list<const char*> triggers, std::initializer_list<const char*> aliases)
	{
		for (const auto* trigger : triggers)
		{
			if (containsNormalizedToken(tokens, trigger))
			{
				for (const auto* alias : aliases)
				{
					appendUniqueToken(tokens, normalizeText(alias));
				}
				return;
			}
		}
	}

	// Check if any of the specified trigger phrases are present in the list of tokens, and if so, append the corresponding alias phrases to the list of tokens. This is similar to addSemanticGroup but operates on a map of domain signals, allowing for more detailed tracking of which domains are relevant based on the presence of certain key terms.
	void addDomainSignal(DomainSignalMap& signals, const std::string& domain, const std::vector<std::string>& tokens, std::initializer_list<const char*> needles)
	{
		for (const auto* needle : needles)
		{
			const std::string normalizedNeedle = normalizeText(needle);
			for (const auto& token : tokens)
			{
				if (token == normalizedNeedle)
				{
					auto& signal = signals[domain];
					++signal.matchCount;
					appendUniqueToken(signal.matchedTerms, normalizedNeedle);
					break;
				}
			}
		}
	}

	// Collect domain signals based on the presence of specific keywords in the list of tokens, allowing for the identification of relevant domains (e.g., finance, healthcare, cybersecurity) based on the candidate's profile and the search criteria. This helps improve the relevance of search results by considering which domains are most relevant to the candidate's profile and the search query.
	DomainSignalMap collectDomainSignals(const std::vector<std::string>& tokens)
	{
		DomainSignalMap signals;
		addDomainSignal(signals, "finance", tokens, { "finance", "financial", "risk", "fraud", "forecasting", "banking" });
		addDomainSignal(signals, "healthcare", tokens, { "healthcare", "hl7", "clinical", "informatics", "health", "health systems" });
		addDomainSignal(signals, "cybersecurity", tokens, { "cybersecurity", "siem", "splunk", "incident response", "network security", "soc", "iam" });
		addDomainSignal(signals, "ux", tokens, { "ux", "figma", "wireframing", "prototyping", "accessibility", "design systems", "user research" });
		addDomainSignal(signals, "marketing", tokens, { "seo", "marketing", "crm", "content", "campaign", "google analytics", "paid search", "lead generation" });
		addDomainSignal(signals, "retail", tokens, { "retail", "merchandising", "merchandise", "pos", "store", "retail operations" });
		addDomainSignal(signals, "logistics", tokens, { "supply chain", "procurement", "inventory control", "warehouse", "fleet", "logistics", "erp", "warehouse planning" });
		addDomainSignal(signals, "media", tokens, { "video", "media", "premiere", "after effects", "audition", "storyboarding", "audio mixing", "production" });
		addDomainSignal(signals, "environmental", tokens, { "environmental", "gis", "sampling", "sustainability", "regulatory reporting", "environmental monitoring", "compliance" });
		addDomainSignal(signals, "automation", tokens, { "plc", "scada", "autocad", "automation", "manufacturing", "iot", "maintenance" });
		addDomainSignal(signals, "software", tokens, { "javascript", "typescript", "react", "node", "html", "css", "c++", "c#", "postgresql", "software", "developer", "frontend", "backend", "full stack", "api", "aws", "azure" });
		addDomainSignal(signals, "data", tokens, { "python", "pandas", "machine learning", "analytics", "power bi", "tableau", "statistics", "data science", "data analyst" });
		return signals;
	}

	// Collect domain signals specifically for role family classification, based on the presence of specific keywords in the list of tokens. This allows for the identification of relevant role families (e.g., software engineering, data science, marketing) based on the candidate's profile and the search criteria, improving the relevance of search results by considering which role families are most relevant to the candidate's profile and the search query.
	DomainSignalMap collectRoleFamilySignals(const std::vector<std::string>& tokens)
	{
		DomainSignalMap signals;
		addDomainSignal(signals, "finance", tokens, { "finance", "financial", "risk", "fraud", "banking" });
		addDomainSignal(signals, "healthcare", tokens, { "healthcare", "hl7", "clinical", "informatics", "health systems" });
		addDomainSignal(signals, "cybersecurity", tokens, { "cybersecurity", "siem", "splunk", "incident response", "network security", "iam" });
		addDomainSignal(signals, "marketing", tokens, { "marketing", "seo", "crm", "content", "campaign", "lead generation", "google analytics", "paid search" });
		addDomainSignal(signals, "retail", tokens, { "retail", "merchandising", "store", "pos", "retail operations" });
		addDomainSignal(signals, "logistics", tokens, { "supply chain", "logistics", "warehouse", "procurement", "inventory control" });
		addDomainSignal(signals, "media", tokens, { "media", "video", "production", "premiere", "after effects" });
		addDomainSignal(signals, "environmental", tokens, { "environmental", "gis", "sustainability", "sampling", "compliance" });
		addDomainSignal(signals, "automation", tokens, { "automation", "plc", "scada", "autocad", "manufacturing", "iot" });
		addDomainSignal(signals, "fullstack", tokens, { "full stack", "full stack developer", "node" });
		addDomainSignal(signals, "frontend", tokens, { "frontend", "react", "javascript", "typescript", "html", "css", "ui", "web" });
		addDomainSignal(signals, "backend", tokens, { "backend", "backend developer", "backend engineer", "api", "rest", "apis", "c++", "c#", "dotnet", "postgresql", "database", "services" });
		addDomainSignal(signals, "ux", tokens, { "ux", "figma", "wireframing", "prototyping", "design systems", "user research", "accessibility" });
		addDomainSignal(signals, "data", tokens, { "python", "pandas", "power bi", "tableau", "statistics", "machine learning", "data science", "data analyst", "analytics", "reporting" });
		return signals;
	}

	// Determine the dominant domain based on the collected domain signals, by identifying which domain has the highest match count. This allows for the classification of candidates into specific domains or role families based on the presence of relevant keywords in their profiles, improving the relevance of search results and enabling more targeted candidate recommendations.
	std::string getDominantDomain(const DomainSignalMap& signals)
	{
		std::string bestDomain;
		int bestCount = 0;
		for (const auto& [domain, signal] : signals)
		{
			if (signal.matchCount > bestCount)
			{
				bestDomain = domain;
				bestCount = signal.matchCount;
			}
		}
		return bestDomain;
	}

	// Get the count of matched signals for a specific domain from the domain signal map, allowing for the quantification of how strongly a candidate's profile matches a particular domain or role family based on the presence of relevant keywords.
	int getDomainSignalCount(const DomainSignalMap& signals, const std::string& domain)
	{
		auto it = signals.find(domain);
		return it == signals.end() ? 0 : it->second.matchCount;
	}

	// Get the list of matched terms for a specific domain from the domain signal map, allowing for the identification of which specific keywords contributed to the match for a particular domain or role family.
	std::vector<std::string> getSemanticConceptTokens(const std::vector<std::string>& rawTokens)
	{
		auto tokens = rawTokens;
		addSemanticGroup(tokens, { "software engineering", "computer science", "information systems", "software", "developer", "backend", "backend developer", "backend engineer", "api", "rest", "apis", "postgresql", "sql", "database", "c++", "c#", "dotnet", "azure", "aws", "cloud engineer" }, { "software", "backend", "services", "api", "database", "engineering" });
		addSemanticGroup(tokens, { "frontend", "react", "javascript", "typescript", "html", "css" }, { "software", "frontend", "web", "ui" });
		addSemanticGroup(tokens, { "full stack", "node", "full stack developer" }, { "software", "backend", "frontend", "web", "api" });
		addSemanticGroup(tokens, { "python", "pandas", "tableau", "power bi", "statistics", "machine learning", "data science", "data analyst", "analytics" }, { "data", "analytics", "reporting" });
		addSemanticGroup(tokens, { "python", "sql", "postgresql", "database", "etl", "data engineering" }, { "database", "backend", "services" });
		addSemanticGroup(tokens, { "retail management", "retail operations", "retail", "merchandising", "pos", "store", "inventory audits", "rostering" }, { "retail", "operations", "store" });
		addSemanticGroup(tokens, { "supply chain", "supply chain management", "procurement", "inventory control", "warehouse", "fleet", "logistics", "erp", "warehouse planning", "demand planning" }, { "logistics", "operations", "planning", "inventory" });
		addSemanticGroup(tokens, { "healthcare", "health systems", "healthcare systems", "hl7", "clinical", "informatics" }, { "healthcare", "clinical", "systems" });
		addSemanticGroup(tokens, { "finance", "financial", "risk", "fraud", "forecasting", "banking", "fraud detection" }, { "finance", "risk", "analytics" });
		addSemanticGroup(tokens, { "cybersecurity", "siem", "splunk", "incident response", "network security", "soc", "iam" }, { "cybersecurity", "security", "monitoring" });
		addSemanticGroup(tokens, { "ux", "figma", "wireframing", "prototyping", "design systems", "user research", "accessibility" }, { "ux", "design", "research", "frontend", "web", "ui" });
		addSemanticGroup(tokens, { "plc", "scada", "autocad", "automation", "manufacturing", "iot", "maintenance" }, { "automation", "engineering", "operations" });
		addSemanticGroup(tokens, { "seo", "marketing", "crm", "content", "campaign", "lead generation", "google analytics", "paid search" }, { "marketing", "growth", "analytics" });
		addSemanticGroup(tokens, { "video", "media", "premiere", "after effects", "storyboarding", "audio mixing", "production", "film and media production" }, { "media", "production", "creative" });
		addSemanticGroup(tokens, { "environmental", "environmental science", "gis", "sampling", "sustainability", "regulatory reporting", "environmental monitoring", "compliance" }, { "environmental", "compliance", "monitoring" });
		return tokens;
	}

	// Check if two tokens match each other, either by being exactly the same or by one being a substring of the other (for tokens of length 4 or more). This allows for flexible matching of related terms and concepts, improving the relevance of search results by considering semantic similarities between tokens.
	bool tokensMatch(const std::string& left, const std::string& right)
	{
		if (left == right)
		{
			return true;
		}
		if (left.size() < 4 || right.size() < 4)
		{
			return false;
		}
		return left.find(right) != std::string::npos || right.find(left) != std::string::npos;
	}

	// Calculate the semantic coverage ratio between a source string and a target string, by comparing the semantic concept tokens extracted from both strings and determining how many of the target tokens are matched by the source tokens. This provides a measure of how well the source string covers the concepts present in the target string, which can be used for scoring and ranking candidates based on their relevance to a job description or search query.
	float getSemanticCoverageRatio(const std::string& source, const std::string& target)
	{
		auto sourceTokens = getSemanticConceptTokens(splitTerms(source));
		auto targetTokens = getSemanticConceptTokens(splitTerms(target));
		if (sourceTokens.empty() || targetTokens.empty())
		{
			return 0.0f;
		}
		int matchedTokens = 0;
		for (const auto& targetToken : targetTokens)
		{
			for (const auto& sourceToken : sourceTokens)
			{
				if (tokensMatch(sourceToken, targetToken))
				{
					++matchedTokens;
					break;
				}
			}
		}
		return static_cast<float>(matchedTokens) / static_cast<float>(targetTokens.size());
	}

	// Check if the list of tokens contains any of the specified needles, allowing for flexible matching of search queries against candidate profiles and job descriptions. This helps improve the relevance of search results by considering multiple related terms and phrases.
	bool containsAnyToken(const std::vector<std::string>& haystack, std::initializer_list<const char*> needles)
	{
		for (const auto* needle : needles)
		{
			const std::string normalizedNeedle = normalizeText(needle);
			for (const auto& token : haystack)
			{
				if (token == normalizedNeedle)
				{
					return true;
				}
			}
		}
		return false;
	}


	std::string detectCandidateDomain(const S_CandidateProfile& candidate)
	{
		return getDominantDomain(collectDomainSignals(splitTerms(candidate.major + " " + candidate.skills + " " + candidate.summary + " " + candidate.workExperience)));
	}

	std::string detectJobDomain(const S_JobCard& job)
	{
		return getDominantDomain(collectDomainSignals(splitTerms(job.title + " " + job.requiredSkills + " " + job.jobDescription)));
	}

	std::string detectCandidateRoleFamily(const S_CandidateProfile& candidate)
	{
		return getDominantDomain(collectRoleFamilySignals(splitTerms(candidate.major + " " + candidate.skills + " " + candidate.summary + " " + candidate.workExperience)));
	}

	std::string detectJobRoleFamily(const S_JobCard& job)
	{
		return getDominantDomain(collectRoleFamilySignals(splitTerms(job.title + " " + job.requiredSkills + " " + job.jobDescription)));
	}


	// Determine the relationship between two domains based on predefined rules, allowing for the classification of how closely related two domains are (e.g., adjacent, related, or none). This can be used to adjust scoring and ranking of candidates based on how closely their detected domain matches the domain of the job description.
	DomainRelationship getDomainRelationship(const std::string& leftDomain, const std::string& rightDomain)
	{
		if (leftDomain.empty() || rightDomain.empty() || leftDomain == rightDomain) return DomainRelationship::None;
		if ((leftDomain == "data" && rightDomain == "software") || (leftDomain == "software" && rightDomain == "data") || (leftDomain == "ux" && rightDomain == "software") || (leftDomain == "software" && rightDomain == "ux")) return DomainRelationship::Adjacent;
		if ((leftDomain == "finance" && rightDomain == "data") || (leftDomain == "data" && rightDomain == "finance") || (leftDomain == "healthcare" && rightDomain == "data") || (leftDomain == "data" && rightDomain == "healthcare") || (leftDomain == "healthcare" && rightDomain == "software") || (leftDomain == "software" && rightDomain == "healthcare") || (leftDomain == "automation" && rightDomain == "software") || (leftDomain == "software" && rightDomain == "automation") || (leftDomain == "cybersecurity" && rightDomain == "software") || (leftDomain == "software" && rightDomain == "cybersecurity")) return DomainRelationship::Related;
		return DomainRelationship::None;
	}

	// Determine the relationship between two role families based on predefined rules, allowing for the classification of how closely related two role families are (e.g., adjacent, related, or none). This can be used to adjust scoring and ranking of candidates based on how closely their detected role family matches the role family of the job description.
	RoleFamilyRelationship getRoleFamilyRelationship(const std::string& leftFamily, const std::string& rightFamily)
	{
		if (leftFamily.empty() || rightFamily.empty() || leftFamily == rightFamily) return RoleFamilyRelationship::None;
		if ((leftFamily == "frontend" && rightFamily == "ux") || (leftFamily == "ux" && rightFamily == "frontend") || (leftFamily == "frontend" && rightFamily == "fullstack") || (leftFamily == "fullstack" && rightFamily == "frontend") || (leftFamily == "backend" && rightFamily == "fullstack") || (leftFamily == "fullstack" && rightFamily == "backend")) return RoleFamilyRelationship::Adjacent;
		if ((leftFamily == "frontend" && rightFamily == "backend") || (leftFamily == "backend" && rightFamily == "frontend") || (leftFamily == "ux" && rightFamily == "fullstack") || (leftFamily == "fullstack" && rightFamily == "ux") || (leftFamily == "backend" && rightFamily == "data") || (leftFamily == "data" && rightFamily == "backend") || (leftFamily == "data" && rightFamily == "fullstack") || (leftFamily == "fullstack" && rightFamily == "data") || (leftFamily == "finance" && rightFamily == "data") || (leftFamily == "data" && rightFamily == "finance")) return RoleFamilyRelationship::Related;
		return RoleFamilyRelationship::None;
	}

	// Calculate a coverage lane score based on the coverage ratio, minimum coverage threshold, full score coverage threshold, and maximum score. This allows for the scoring of candidates based on how well their profiles cover the relevant concepts in the job description, with a smooth scaling of scores between the minimum and full coverage thresholds.
	float getCoverageLaneScore(float coverage, float minimumCoverage, float fullScoreCoverage, float maxScore)
	{
		if (coverage < minimumCoverage) return 0.0f;
		if (coverage >= fullScoreCoverage) return maxScore;
		return maxScore * ((coverage - minimumCoverage) / (fullScoreCoverage - minimumCoverage));
	}

	// Parse a numeric value from a string by extracting digits and decimal points, allowing for the conversion of strings that may contain non-numeric characters into a usable numeric format. This can be used for interpreting salary ranges, years of experience, or other numeric criteria from candidate profiles and job descriptions.
	double parseNumericValue(const std::string& value)
	{
		std::string digits;
		digits.reserve(value.size());
		for (unsigned char ch : value)
		{
			if (std::isdigit(ch) || ch == '.') digits.push_back(static_cast<char>(ch));
		}
		if (digits.empty()) return 0.0;
		try { return std::stod(digits); }
		catch (...) { return 0.0; }
	}

	// Calculate the coverage ratio between a source string and a target string, by comparing the tokens extracted from both strings and determining how many of the target tokens are matched by the source tokens. This provides a measure of how well the source string covers the concepts present in the target string, which can be used for scoring and ranking candidates based on their relevance to a job description or search query.
	float getCoverageRatio(const std::string& source, const std::string& target)
	{
		const auto sourceTokens = splitTerms(source);
		const auto targetTokens = splitTerms(target);
		if (sourceTokens.empty() || targetTokens.empty()) return 0.0f;
		int matchedTokens = 0;
		for (const auto& targetToken : targetTokens)
		{
			for (const auto& sourceToken : sourceTokens)
			{
				if (tokensMatch(sourceToken, targetToken))
				{
					++matchedTokens;
					break;
				}
			}
		}
		return static_cast<float>(matchedTokens) / static_cast<float>(targetTokens.size());
	}

	// Calculate a domain alignment score between a candidate profile and a job card, based on the detected domains and role families, the semantic overlap of their relevant fields, and the strength of domain signals. This allows for a nuanced scoring of how well a candidate's profile aligns with the domain and role family of a job description, improving the relevance of search results and candidate recommendations.
	float getDomainAlignmentScore(const S_CandidateProfile& candidate, const S_JobCard& job)
	{
		const DomainSignalMap candidateSignals = collectDomainSignals(splitTerms(candidate.major + " " + candidate.skills + " " + candidate.summary + " " + candidate.workExperience));
		const DomainSignalMap jobSignals = collectDomainSignals(splitTerms(job.title + " " + job.requiredSkills + " " + job.jobDescription));
		const std::string candidateDomain = detectCandidateDomain(candidate);
		const std::string jobDomain = detectJobDomain(job);
		const std::string candidateFamily = detectCandidateRoleFamily(candidate);
		const std::string jobFamily = detectJobRoleFamily(job);
		const std::string candidateContext = candidate.major + " " + candidate.skills + " " + candidate.summary + " " + candidate.workExperience;
		const std::string jobContext = job.title + " " + job.requiredSkills + " " + job.jobDescription;
		const float semanticOverlap = std::max(getSemanticCoverageRatio(candidateContext, jobContext), getSemanticCoverageRatio(jobContext, candidateContext));
		const DomainRelationship relationship = getDomainRelationship(candidateDomain, jobDomain);
		const RoleFamilyRelationship familyRelationship = getRoleFamilyRelationship(candidateFamily, jobFamily);
		const int candidateDomainStrength = getDomainSignalCount(candidateSignals, candidateDomain);
		const int jobDomainStrength = getDomainSignalCount(jobSignals, jobDomain);
		const bool strongDomainEvidence = candidateDomainStrength >= 2 && jobDomainStrength >= 2;
		if (candidateDomain.empty() || jobDomain.empty())
		{
			return semanticOverlap >= 0.5f ? 10.0f : 0.0f;
		}
		if (!candidateFamily.empty() && candidateFamily == jobFamily) return 28.0f;
		if (candidateDomain == jobDomain && strongDomainEvidence && (candidateFamily.empty() || jobFamily.empty())) return 28.0f;
		if (familyRelationship == RoleFamilyRelationship::Adjacent && candidateDomain == jobDomain && semanticOverlap >= 0.18f) return 18.0f;
		if (relationship == DomainRelationship::Adjacent && semanticOverlap >= 0.5f) return 16.0f;
		if (familyRelationship == RoleFamilyRelationship::Adjacent && candidateDomain != jobDomain && semanticOverlap >= 0.18f) return 15.0f;
		if (semanticOverlap >= 0.6f) return 14.0f;
		if (familyRelationship == RoleFamilyRelationship::Adjacent && candidateDomain == jobDomain) return 12.0f;
		if (familyRelationship == RoleFamilyRelationship::Related && candidateDomain == jobDomain && semanticOverlap >= 0.18f) return 12.0f;
		if (familyRelationship == RoleFamilyRelationship::Adjacent && candidateDomain != jobDomain) return 10.0f;
		if (familyRelationship == RoleFamilyRelationship::Related && candidateDomain != jobDomain && semanticOverlap >= 0.18f) return 10.0f;
		if (relationship == DomainRelationship::Related && semanticOverlap >= 0.45f) return 10.0f;
		if (familyRelationship == RoleFamilyRelationship::Related && candidateDomain == jobDomain) return 8.0f;
		if (relationship == DomainRelationship::Adjacent && semanticOverlap >= 0.32f) return 8.0f;
		if (familyRelationship == RoleFamilyRelationship::Related && candidateDomain != jobDomain) return 6.0f;
		if (semanticOverlap >= 0.45f) return 6.0f;
		if (relationship == DomainRelationship::Related && semanticOverlap >= 0.24f) return 4.0f;
		return -55.0f;
	}

	// Calculate a work mode score between a candidate profile and a job card, based on the candidate's preferred work mode and the job's work mode. This allows for scoring of candidates based on their alignment with the work mode requirements of the job description, improving the relevance of search results and candidate recommendations.
	float getWorkModeScore(const S_CandidateProfile& candidate, const S_JobCard& job)
	{
		if (candidate.preferredWorkMode.empty() || job.workMode.empty()) return 0.0f;
		return normalizeText(candidate.preferredWorkMode) == normalizeText(job.workMode) ? 4.0f : 0.0f;
	}

	// Calculate a location score between a candidate profile and a job card, based on the candidate's preferred location and the job's location. This allows for scoring of candidates based on their alignment with the location requirements of the job description, improving the relevance of search results and candidate recommendations.
	float getLocationScore(const S_CandidateProfile& candidate, const S_JobCard& job)
	{
		if (candidate.preferredLocation.empty() || job.location.empty()) return 0.0f;
		const float coverage = std::max({ getCoverageRatio(job.location, candidate.preferredLocation), getCoverageRatio(candidate.preferredLocation, job.location), getSemanticCoverageRatio(job.location, candidate.preferredLocation), getSemanticCoverageRatio(candidate.preferredLocation, job.location) });
		return coverage >= 0.85f ? 4.0f : 4.0f * coverage;
	}

	// Calculate an education score between a candidate profile and a job card, based on the candidate's education level and the job's required education level. This allows for scoring of candidates based on their alignment with the education requirements of the job description, improving the relevance of search results and candidate recommendations.
	float getEducationScore(const S_CandidateProfile& candidate, const S_JobCard& job)
	{
		const int requiredRank = educationLevelRank(educationLevelFromString(job.requiredEducation));
		if (requiredRank == 0) return 3.0f;
		const int candidateRank = educationLevelRank(educationLevelFromString(candidate.education));
		return candidateRank >= requiredRank ? 3.0f : 0.0f;
	}

	// Calculate an experience score between a candidate profile and a job card, based on the candidate's years of experience and the job's required years of experience. This allows for scoring of candidates based on their alignment with the experience requirements of the job description, improving the relevance of search results and candidate recommendations.
	float getExperienceScore(const S_CandidateProfile& candidate, const S_JobCard& job)
	{
		if (job.requiredExperience <= 0) return 3.0f;
		if (candidate.yearsExperience <= 0) return 0.0f;
		if (candidate.yearsExperience >= job.requiredExperience) return 3.0f;
		return 3.0f * (static_cast<float>(candidate.yearsExperience) / static_cast<float>(job.requiredExperience));
	}

	// Calculate a salary fit score between a candidate profile and a job card, based on the candidate's expected salary and the job's salary range. This allows for scoring of candidates based on their alignment with the salary requirements of the job description, improving the relevance of search results and candidate recommendations.
	float getSalaryFitScore(const S_CandidateProfile& candidate, const S_JobCard& job)
	{
		const double expectedSalary = parseNumericValue(candidate.expectedSalary);
		const double salaryMin = parseNumericValue(job.salaryMin);
		const double salaryMax = parseNumericValue(job.salaryMax);
		if (expectedSalary <= 0.0 || (salaryMin <= 0.0 && salaryMax <= 0.0)) return 0.0f;
		if (salaryMin > 0.0 && salaryMax > 0.0)
		{
			if (expectedSalary >= salaryMin && expectedSalary <= salaryMax) return 5.0f;
			if (expectedSalary < salaryMin) return static_cast<float>(5.0 * (expectedSalary / salaryMin));
			return static_cast<float>(5.0 * (salaryMax / expectedSalary));
		}
		if (salaryMax > 0.0) return expectedSalary <= salaryMax ? 5.0f : static_cast<float>(5.0 * (salaryMax / expectedSalary));
		return expectedSalary >= salaryMin ? 5.0f : static_cast<float>(5.0 * (expectedSalary / salaryMin));
	}

	// Format a match reason string with a category, tier, and message, allowing for the generation of user-friendly explanations for why a candidate matches or does not match a job description based on specific criteria. This can be used to provide feedback to employers and candidates about the strengths and weaknesses of a match.
	std::string formatTieredReason(const std::string& category, const char* tier, const std::string& message)
	{
		return category + " - " + tier + ": " + message;
	}

	// Build a list of match reasons for a candidate and a job, based on various scoring criteria and the specified audience (employer or candidate). This allows for the generation of detailed explanations for why a candidate matches or does not match a job description, tailored to the perspective of either the employer or the candidate.
	std::vector<std::string> buildMatchReasons(const S_CandidateProfile& candidate, const S_JobCard& job, MatchReasonAudience audience)
	{
		std::vector<std::string> reasons;
		const bool employerAudience = audience == MatchReasonAudience::Employer;
		const float skillScore = ScoringLogic::GetSkillMatchScore(candidate, job);
		const float fieldScore = ScoringLogic::GetFieldAlignmentScore(candidate, job);
		const float domainScore = getDomainAlignmentScore(candidate, job);
		const float majorScore = ScoringLogic::GetMajorRelevanceScore(candidate, job);
		const float workModeScore = getWorkModeScore(candidate, job);
		const float locationScore = getLocationScore(candidate, job);
		const float educationScore = getEducationScore(candidate, job);
		const float experienceScore = getExperienceScore(candidate, job);
		const float salaryScore = getSalaryFitScore(candidate, job);
		const float coreScore = getDomainAlignmentScore(candidate, job) + ScoringLogic::GetFieldAlignmentScore(candidate, job) + ScoringLogic::GetSkillMatchScore(candidate, job) + ScoringLogic::GetMajorRelevanceScore(candidate, job);

		if (domainScore > 0.0f) reasons.push_back(formatTieredReason("Domain Alignment", "High", employerAudience ? "This candidate is in the same domain as this job's requirements and responsibilities." : "This role is in the same domain as your profile and experience."));
		else if (domainScore == 0.0f) reasons.push_back(formatTieredReason("Domain Alignment", "Medium", employerAudience ? "The candidate's domain overlap with this job is limited or unclear from the current profile." : "The domain overlap is limited or unclear from your current profile."));
		else reasons.push_back(formatTieredReason("Domain Alignment", "Low", employerAudience ? "This candidate appears to specialize outside this job's main domain." : "This role appears to sit outside your main domain or specialization."));

		if (domainScore > 0.0f && skillScore >= 20.0f) reasons.push_back(formatTieredReason("Skills Match", "High", employerAudience ? "The candidate's skills align strongly with the required skills for this role." : "Your skills align strongly with the required skills for this role."));
		else if (domainScore >= 0.0f && skillScore >= 8.0f) reasons.push_back(formatTieredReason("Skills Match", "Medium", employerAudience ? "The candidate has some relevant skills, but the overlap is only partial." : "You have some relevant skills, but the overlap is only partial."));
		else reasons.push_back(formatTieredReason("Skills Match", "Low", employerAudience ? "The candidate's current skills have limited overlap with this role's requirements." : "Your current skills have limited overlap with this role's requirements."));

		if (fieldScore >= 9.0f && domainScore > 0.0f) reasons.push_back(formatTieredReason("Role Responsibilities", "High", employerAudience ? "The candidate's background and experience align well with this job's responsibilities." : "Your background and experience align well with the role responsibilities."));
		else if (domainScore >= 0.0f && fieldScore >= 3.0f) reasons.push_back(formatTieredReason("Role Responsibilities", "Medium", employerAudience ? "Some parts of the candidate's background relate to the responsibilities in this job." : "Some parts of your background relate to the responsibilities in this job."));
		else reasons.push_back(formatTieredReason("Role Responsibilities", "Low", employerAudience ? "The candidate's profile shows limited evidence of experience in this job's main responsibilities." : "Your profile shows limited evidence of experience in this job's main responsibilities."));

		if (majorScore >= 3.0f && domainScore > 0.0f) reasons.push_back(formatTieredReason("Major Relevance", "High", employerAudience ? "The candidate's major is directly relevant to this position." : "Your major is directly relevant to this position."));
		else if (domainScore >= 0.0f && majorScore >= 1.5f) reasons.push_back(formatTieredReason("Major Relevance", "Medium", employerAudience ? "The candidate's major has some relevance to this role." : "Your major has some relevance to this role."));
		else reasons.push_back(formatTieredReason("Major Relevance", "Low", employerAudience ? "The candidate's major does not closely align with this role." : "Your major does not closely align with this role."));

		const float qualificationsScore = educationScore + experienceScore;
		if (domainScore > 0.0f && coreScore > 18.0f && qualificationsScore >= 5.0f) reasons.push_back(formatTieredReason("Qualifications Fit", "High", employerAudience ? "The candidate's education and experience meet or exceed the role requirements." : "Your education and experience meet or exceed the role requirements."));
		else if (domainScore >= 0.0f && coreScore > 8.0f && qualificationsScore >= 2.5f) reasons.push_back(formatTieredReason("Qualifications Fit", "Medium", employerAudience ? "The candidate meets some of the education or experience expectations for this role." : "You meet some of the education or experience expectations for this role."));
		else reasons.push_back(formatTieredReason("Qualifications Fit", "Low", employerAudience ? "The candidate's education and experience are not a strong match for this role's stated requirements." : "Your education and experience are not a strong match for this role's stated requirements."));

		const float preferenceScore = workModeScore + locationScore + salaryScore;
		if (coreScore > 20.0f && preferenceScore >= 7.0f) reasons.push_back(formatTieredReason("Preference Fit", "High", employerAudience ? "This role aligns well with the candidate's preferred work setup, location, or salary expectations." : "This role aligns well with your preferred work setup, location, or salary expectations."));
		else if (coreScore > 10.0f && preferenceScore >= 3.0f) reasons.push_back(formatTieredReason("Preference Fit", "Medium", employerAudience ? "Some of the candidate's work preferences match this role." : "Some of your work preferences match this role."));
		else reasons.push_back(formatTieredReason("Preference Fit", "Low", employerAudience ? "This role is not a strong fit for the candidate's stated work preferences." : "This role is not a strong fit for your stated work preferences."));

		return reasons;
	}

	// Convert a candidate card to a candidate profile by mapping the relevant fields, allowing for the use of a more detailed candidate profile structure in scoring and matching logic while still being able to accept input in the form of a simpler candidate card.
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
}

// Calculate a keyword relevance score for a job card based on the presence of the keyword in various fields of the job card, as well as fuzzy matching scores. This allows for scoring of how well a job card matches a search query based on the presence and relevance of the search terms in key fields of the job description.
float ScoringLogic::GetJobKeywordScore(const S_JobCard& job, const std::string& keyword)
{
	float score = 0.0f;
	const auto queries = expandJobSearchQueries(keyword);
	for (const auto& query : queries.phraseQueries)
	{
		if (containsKeywordTerm(job.title, query)) score += 200.0f;
		if (containsKeywordTerm(job.requiredSkills, query)) score += 80.0f;
		if (containsKeywordTerm(job.jobDescription, query)) score += 50.0f;
		score += (getFuzzyScore(job.title, query) * 4.0f)
			+ (getFuzzyScore(job.company, query) * 1.0f)
			+ (getFuzzyScore(job.requiredSkills, query) * 2.0f)
			+ (getFuzzyScore(job.jobDescription, query) * 2.0f)
			+ (getFuzzyScore(job.location, query) * 0.5f)
			+ (getFuzzyScore(job.type, query) * 0.5f)
			+ (getFuzzyScore(job.workMode, query) * 0.5f);
	}

	for (const auto& query : queries.tokenQueries)
	{
		if (containsWholeSearchTerm(job.title, query)) score += 200.0f;
		if (containsWholeSearchTerm(job.requiredSkills, query)) score += 80.0f;
		if (containsWholeSearchTerm(job.jobDescription, query)) score += 50.0f;
		score += (getFuzzyScore(job.title, query) * 4.0f)
			+ (getFuzzyScore(job.company, query) * 1.0f)
			+ (getFuzzyScore(job.requiredSkills, query) * 2.0f)
			+ (getFuzzyScore(job.jobDescription, query) * 2.0f)
			+ (getFuzzyScore(job.location, query) * 0.5f)
			+ (getFuzzyScore(job.type, query) * 0.5f)
			+ (getFuzzyScore(job.workMode, query) * 0.5f);
	}

	return score;
}

// Check if a job card has a fuzzy keyword match for a given search query, by checking for the presence of the keyword in various fields of the job card and allowing for fuzzy matching. This allows for a flexible determination of whether a job card is relevant to a search query, even if the query does not exactly match the text in the job description.
bool ScoringLogic::HasFuzzyKeywordMatch(const S_JobCard& job, const std::string& keyword)
{
	if (keyword.empty()) return true;
	const auto queries = expandJobSearchQueries(keyword);
	for (const auto& query : queries.phraseQueries)
	{
		if (containsKeywordTerm(job.title, query)
			|| containsKeywordTerm(job.company, query)
			|| containsKeywordTerm(job.requiredSkills, query)
			|| containsKeywordTerm(job.jobDescription, query))
		{
			return true;
		}
	}

	for (const auto& query : queries.tokenQueries)
	{
		if (containsWholeSearchTerm(job.title, query)
			|| containsWholeSearchTerm(job.company, query)
			|| containsApproximateWholeSearchTerm(job.company, query)
			|| containsWholeSearchTerm(job.requiredSkills, query)
			|| containsWholeSearchTerm(job.jobDescription, query)
			|| containsWholeSearchTerm(job.title, query)
			|| containsWholeSearchTerm(job.requiredSkills, query))
		{
			return true;
		}
	}

	return false;
}

// Calculate a search relevance score for a user summary based on the presence of the search query in various fields of the user summary, as well as fuzzy matching scores. This allows for scoring of how well a user summary matches a search query based on the presence and relevance of the search terms in key fields of the user's profile.
float ScoringLogic::GetUserSearchScore(const S_UserSummary& user, const std::string& query)
{
	const std::string normalizedQuery = normalizeSearchText(query);
	if (normalizedQuery.empty())
	{
		return 0.0f;
	}

	float score = 0.0f;

	if (containsKeywordTerm(user.fullName, normalizedQuery)) score += 140.0f;
	if (containsKeywordTerm(user.companyName, normalizedQuery)) score += 160.0f;
	if (containsKeywordTerm(user.email, normalizedQuery)) score += 120.0f;

	if (containsWholeSearchTerm(user.fullName, normalizedQuery)) score += 110.0f;
	else if (containsApproximateWholeSearchTerm(user.fullName, normalizedQuery)) score += 85.0f;

	if (containsWholeSearchTerm(user.companyName, normalizedQuery)) score += 140.0f;
	else if (containsApproximateWholeSearchTerm(user.companyName, normalizedQuery)) score += 120.0f;

	if (containsWholeSearchTerm(user.email, normalizedQuery)) score += 90.0f;
	else if (containsApproximateWholeSearchTerm(user.email, normalizedQuery)) score += 65.0f;

	score += (getFuzzyScore(user.fullName, normalizedQuery) * 3.0f)
		+ (getFuzzyScore(user.companyName, normalizedQuery) * 3.5f)
		+ (getFuzzyScore(user.email, normalizedQuery) * 2.0f)
		+ getFuzzyScore(user.role, normalizedQuery)
		+ getFuzzyScore(user.profileOrJobs, normalizedQuery)
		+ getFuzzyScore(user.membershipStatus, normalizedQuery);

	return score;
}

// Calculate a search relevance score for a candidate card based on the presence of the search query and skills in various fields of the candidate card, as well as fuzzy matching scores. This allows for scoring of how well a candidate card matches a search query based on the presence and relevance of the search terms in key fields of the candidate's profile.
float ScoringLogic::GetCandidateSearchScore(const S_CandidateCard& candidate, const std::string& keyword, const std::string& skills)
{
	float score = 0.0f;
	if (!keyword.empty())
	{
		const auto queries = expandJobSearchQueries(keyword);
		for (const auto& query : queries.phraseQueries)
		{
			if (containsKeywordTerm(candidate.fullName, query)) score += 140.0f;
			if (containsKeywordTerm(candidate.major, query)) score += 90.0f;
			if (containsKeywordTerm(candidate.skills, query)) score += 110.0f;
			if (containsKeywordTerm(candidate.summary, query)) score += 55.0f;
			if (containsKeywordTerm(candidate.workExperience, query)) score += 40.0f;
			if (containsKeywordTerm(candidate.education, query)) score += 25.0f;
			score += (getFuzzyScore(candidate.fullName, query) * 3.0f)
				+ (getFuzzyScore(candidate.major, query) * 2.5f)
				+ (getFuzzyScore(candidate.skills, query) * 3.0f)
				+ (getFuzzyScore(candidate.summary, query) * 1.5f)
				+ (getFuzzyScore(candidate.workExperience, query) * 1.25f)
				+ getFuzzyScore(candidate.education, query)
				+ getFuzzyScore(candidate.preferredLocation, query)
				+ getFuzzyScore(candidate.preferredWorkMode, query)
				+ getFuzzyScore(candidate.experienceText, query);
		}

		for (const auto& query : queries.tokenQueries)
		{
			if (containsWholeSearchTerm(candidate.fullName, query)) score += 140.0f;
			else if (containsApproximateWholeSearchTerm(candidate.fullName, query)) score += 95.0f;
			if (containsWholeSearchTerm(candidate.major, query)) score += 90.0f;
			else if (containsApproximateWholeSearchTerm(candidate.major, query)) score += 55.0f;
			if (containsWholeSearchTerm(candidate.skills, query)) score += 110.0f;
			else if (containsApproximateWholeSearchTerm(candidate.skills, query)) score += 70.0f;
			if (containsWholeSearchTerm(candidate.summary, query)) score += 55.0f;
			if (containsWholeSearchTerm(candidate.workExperience, query)) score += 40.0f;
			if (containsWholeSearchTerm(candidate.education, query)) score += 25.0f;
			score += (getFuzzyScore(candidate.fullName, query) * 3.0f)
				+ (getFuzzyScore(candidate.major, query) * 2.5f)
				+ (getFuzzyScore(candidate.skills, query) * 3.0f)
				+ (getFuzzyScore(candidate.summary, query) * 1.5f)
				+ (getFuzzyScore(candidate.workExperience, query) * 1.25f)
				+ getFuzzyScore(candidate.education, query)
				+ getFuzzyScore(candidate.preferredLocation, query)
				+ getFuzzyScore(candidate.preferredWorkMode, query)
				+ getFuzzyScore(candidate.experienceText, query);
		}
	}
	if (!skills.empty())
	{
		const auto queries = expandJobSearchQueries(skills);
		for (const auto& query : queries.phraseQueries)
		{
			if (containsKeywordTerm(candidate.skills, query)) score += 120.0f;
			if (containsKeywordTerm(candidate.major, query)) score += 30.0f;
			score += (getFuzzyScore(candidate.skills, query) * 3.0f)
				+ getFuzzyScore(candidate.major, query)
				+ getFuzzyScore(candidate.summary, query);
		}

		for (const auto& query : queries.tokenQueries)
		{
			if (containsWholeSearchTerm(candidate.skills, query)) score += 120.0f;
			else if (containsApproximateWholeSearchTerm(candidate.skills, query)) score += 75.0f;
			if (containsWholeSearchTerm(candidate.major, query)) score += 30.0f;
			else if (containsApproximateWholeSearchTerm(candidate.major, query)) score += 20.0f;
			if (containsWholeSearchTerm(candidate.summary, query)) score += 15.0f;
			score += (getFuzzyScore(candidate.skills, query) * 3.0f)
				+ getFuzzyScore(candidate.major, query)
				+ getFuzzyScore(candidate.summary, query);
		}
	}
	return score;
}

// Check if a candidate card has a fuzzy match for a given search query and skills, by checking for the presence of the keyword and skills in various fields of the candidate card and allowing for fuzzy matching. This allows for a flexible determination of whether a candidate card is relevant to a search query, even if the query does not exactly match the text in the candidate's profile.
bool ScoringLogic::HasFuzzyCandidateMatch(const S_CandidateCard& candidate, const std::string& keyword, const std::string& skills)
{
	if (!keyword.empty())
	{
		const auto queries = expandJobSearchQueries(keyword);
		bool keywordMatch = false;
		for (const auto& query : queries.phraseQueries)
		{
			if (hasCandidateKeywordPhraseMatch(candidate, query))
			{
				keywordMatch = true;
				break;
			}
		}

		if (!keywordMatch)
		{
			for (const auto& query : queries.tokenQueries)
			{
				if (hasCandidateKeywordTokenMatch(candidate, query))
				{
					keywordMatch = true;
					break;
				}
			}
		}

		if (!keywordMatch) return false;
	}
	if (!skills.empty())
	{
		const auto queries = expandJobSearchQueries(skills);
		bool skillsMatch = false;
		for (const auto& query : queries.phraseQueries)
		{
			if (hasCandidateSkillsPhraseMatch(candidate, query))
			{
				skillsMatch = true;
				break;
			}
		}

		if (!skillsMatch)
		{
			for (const auto& query : queries.tokenQueries)
			{
				if (hasCandidateSkillsTokenMatch(candidate, query))
				{
					skillsMatch = true;
					break;
				}
			}
		}

		if (!skillsMatch) return false;
	}
	return true;
}

// Calculate a skill match score for a candidate profile and a job card, based on the coverage of required skills, the semantic overlap of skills, the detected domains and role families, and the strength of domain signals. This allows for a nuanced scoring of how well a candidate's skills align with the requirements of a job description, improving the relevance of search results and candidate recommendations.
float ScoringLogic::GetSkillMatchScore(const S_CandidateProfile& candidate, const S_JobCard& job)
{
	if (job.requiredSkills.empty()) return 0.0f;
	const float coverage = std::max(getCoverageRatio(candidate.skills, job.requiredSkills), getSemanticCoverageRatio(candidate.skills, job.requiredSkills));
	const DomainSignalMap candidateSignals = collectDomainSignals(splitTerms(candidate.major + " " + candidate.skills + " " + candidate.summary + " " + candidate.workExperience));
	const DomainSignalMap jobSignals = collectDomainSignals(splitTerms(job.title + " " + job.requiredSkills + " " + job.jobDescription));
	const DomainSignalMap candidateFamilySignals = collectRoleFamilySignals(splitTerms(candidate.major + " " + candidate.skills + " " + candidate.summary + " " + candidate.workExperience));
	const DomainSignalMap jobFamilySignals = collectRoleFamilySignals(splitTerms(job.title + " " + job.requiredSkills + " " + job.jobDescription));
	const std::string candidateDomain = detectCandidateDomain(candidate);
	const std::string jobDomain = detectJobDomain(job);
	const std::string candidateFamily = detectCandidateRoleFamily(candidate);
	const std::string jobFamily = detectJobRoleFamily(job);
	const bool sameFamily = !candidateFamily.empty() && candidateFamily == jobFamily;
	const DomainRelationship relationship = getDomainRelationship(candidateDomain, jobDomain);
	const RoleFamilyRelationship familyRelationship = getRoleFamilyRelationship(candidateFamily, jobFamily);
	const bool adjacentDomains = relationship == DomainRelationship::Adjacent;
	const bool relatedDomains = relationship == DomainRelationship::Related;
	const bool adjacentFamilies = familyRelationship == RoleFamilyRelationship::Adjacent;
	const bool relatedFamilies = familyRelationship == RoleFamilyRelationship::Related;
	const bool strongDomainEvidence = getDomainSignalCount(candidateSignals, candidateDomain) >= 2 && getDomainSignalCount(jobSignals, jobDomain) >= 2;
	const bool strongFamilyEvidence = getDomainSignalCount(candidateFamilySignals, candidateFamily) >= 2 && getDomainSignalCount(jobFamilySignals, jobFamily) >= 2;
	if (!candidateDomain.empty() && !jobDomain.empty() && candidateDomain != jobDomain && relationship == DomainRelationship::None && coverage < 0.75f) return 0.0f;
		const bool hasRelationshipLane = (sameFamily && strongFamilyEvidence)
			|| (adjacentDomains && strongDomainEvidence)
			|| (adjacentFamilies && strongFamilyEvidence)
			|| (relatedDomains && strongDomainEvidence)
			|| (relatedFamilies && strongFamilyEvidence);

		if (sameFamily && strongFamilyEvidence) return getCoverageLaneScore(coverage, 0.32f, 0.72f, 28.0f);
		if (!hasRelationshipLane) return getCoverageLaneScore(coverage, 0.55f, 0.85f, 28.0f);
		if (adjacentDomains && strongDomainEvidence) return getCoverageLaneScore(coverage, 0.26f, 0.68f, 18.0f);
		if (adjacentFamilies && strongFamilyEvidence) return getCoverageLaneScore(coverage, 0.2f, 0.54f, 14.0f);
		if (relatedDomains && strongDomainEvidence) return getCoverageLaneScore(coverage, 0.3f, 0.64f, 12.0f);
		if (relatedFamilies && strongFamilyEvidence) return getCoverageLaneScore(coverage, 0.26f, 0.6f, 8.0f);
		return 0.0f;
}

// Calculate a field alignment score for a candidate profile and a job card, based on the coverage of relevant fields, the detected domains and role families, and the strength of domain signals. This allows for a nuanced scoring of how well a candidate's overall profile aligns with the requirements and responsibilities of a job description, improving the relevance of search results and candidate recommendations.
float ScoringLogic::GetFieldAlignmentScore(const S_CandidateProfile& candidate, const S_JobCard& job)
{
	const std::string candidateContext = candidate.major + " " + candidate.summary + " " + candidate.workExperience + " " + candidate.skills;
	const std::string jobContext = job.title + " " + job.jobDescription + " " + job.requiredSkills;
	const float coverage = std::max({ getCoverageRatio(candidateContext, jobContext), getCoverageRatio(jobContext, candidateContext), getSemanticCoverageRatio(candidateContext, jobContext), getSemanticCoverageRatio(jobContext, candidateContext) });
	const std::string candidateDomain = detectCandidateDomain(candidate);
	const std::string jobDomain = detectJobDomain(job);
	const std::string candidateFamily = detectCandidateRoleFamily(candidate);
	const std::string jobFamily = detectJobRoleFamily(job);
	const DomainSignalMap candidateSignals = collectDomainSignals(splitTerms(candidateContext));
	const DomainSignalMap jobSignals = collectDomainSignals(splitTerms(jobContext));
	const DomainSignalMap candidateFamilySignals = collectRoleFamilySignals(splitTerms(candidateContext));
	const DomainSignalMap jobFamilySignals = collectRoleFamilySignals(splitTerms(jobContext));
	const bool sameFamily = !candidateFamily.empty() && candidateFamily == jobFamily;
	const DomainRelationship relationship = getDomainRelationship(candidateDomain, jobDomain);
	const RoleFamilyRelationship familyRelationship = getRoleFamilyRelationship(candidateFamily, jobFamily);
	const bool adjacentDomains = relationship == DomainRelationship::Adjacent;
	const bool relatedDomains = relationship == DomainRelationship::Related;
	const bool adjacentFamilies = familyRelationship == RoleFamilyRelationship::Adjacent;
	const bool relatedFamilies = familyRelationship == RoleFamilyRelationship::Related;
	const bool strongDomainEvidence = getDomainSignalCount(candidateSignals, candidateDomain) >= 2 && getDomainSignalCount(jobSignals, jobDomain) >= 2;
	const bool strongFamilyEvidence = getDomainSignalCount(candidateFamilySignals, candidateFamily) >= 2 && getDomainSignalCount(jobFamilySignals, jobFamily) >= 2;
	const bool anyRelationshipLane = sameFamily || adjacentFamilies || relatedFamilies || adjacentDomains || relatedDomains;

	if (sameFamily && strongFamilyEvidence && coverage >= 0.32f) return getCoverageLaneScore(coverage, 0.32f, 0.62f, 12.0f);
	if (!anyRelationshipLane && coverage >= 0.4f) return getCoverageLaneScore(coverage, 0.4f, 0.7f, 12.0f);
	if (adjacentDomains && strongDomainEvidence && coverage >= 0.28f) return getCoverageLaneScore(coverage, 0.28f, 0.58f, 9.0f);
	if (adjacentFamilies && strongFamilyEvidence && coverage >= 0.22f) return getCoverageLaneScore(coverage, 0.22f, 0.48f, 8.5f);
	if (relatedDomains && strongDomainEvidence && coverage >= 0.3f) return getCoverageLaneScore(coverage, 0.3f, 0.6f, 6.5f);
	if (relatedFamilies && strongFamilyEvidence && coverage >= 0.26f) return getCoverageLaneScore(coverage, 0.26f, 0.54f, 5.5f);
	if (sameFamily && strongFamilyEvidence) return 4.0f;
	if (adjacentFamilies && strongFamilyEvidence) return 3.5f;
	if (adjacentDomains && strongDomainEvidence) return 3.0f;
	if (relatedFamilies && strongFamilyEvidence) return 2.5f;
	if (relatedDomains && strongDomainEvidence) return 2.0f;
	if (!anyRelationshipLane)
	{
		if (coverage < 0.22f) return -45.0f;
		return 0.0f;
	}
	return 0.0f;
}

// Calculate a major relevance score for a candidate profile and a job card, based on the coverage of the candidate's major with the job description, the detected domains and role families, and the strength of domain signals. This allows for a nuanced scoring of how well a candidate's educational background aligns with the requirements and responsibilities of a job description, improving the relevance of search results and candidate recommendations.
float ScoringLogic::GetMajorRelevanceScore(const S_CandidateProfile& candidate, const S_JobCard& job)
{
	if (candidate.major.empty()) return 0.0f;
	const float coverage = std::max({ getCoverageRatio(job.title + " " + job.jobDescription + " " + job.requiredSkills, candidate.major), getCoverageRatio(candidate.major, job.title + " " + job.requiredSkills), getSemanticCoverageRatio(job.title + " " + job.jobDescription + " " + job.requiredSkills, candidate.major), getSemanticCoverageRatio(candidate.major, job.title + " " + job.requiredSkills) });
	const std::string candidateDomain = detectCandidateDomain(candidate);
	const std::string jobDomain = detectJobDomain(job);
	const std::string candidateFamily = detectCandidateRoleFamily(candidate);
	const std::string jobFamily = detectJobRoleFamily(job);
	const DomainSignalMap candidateSignals = collectDomainSignals(splitTerms(candidate.major + " " + candidate.skills + " " + candidate.summary + " " + candidate.workExperience));
	const DomainSignalMap jobSignals = collectDomainSignals(splitTerms(job.title + " " + job.requiredSkills + " " + job.jobDescription));
	const DomainSignalMap candidateFamilySignals = collectRoleFamilySignals(splitTerms(candidate.major + " " + candidate.skills + " " + candidate.summary + " " + candidate.workExperience));
	const DomainSignalMap jobFamilySignals = collectRoleFamilySignals(splitTerms(job.title + " " + job.requiredSkills + " " + job.jobDescription));
	const bool sameFamily = !candidateFamily.empty() && candidateFamily == jobFamily;
	const DomainRelationship relationship = getDomainRelationship(candidateDomain, jobDomain);
	const RoleFamilyRelationship familyRelationship = getRoleFamilyRelationship(candidateFamily, jobFamily);
	const bool adjacentDomains = relationship == DomainRelationship::Adjacent;
	const bool relatedDomains = relationship == DomainRelationship::Related;
	const bool adjacentFamilies = familyRelationship == RoleFamilyRelationship::Adjacent;
	const bool relatedFamilies = familyRelationship == RoleFamilyRelationship::Related;
	const bool strongDomainEvidence = getDomainSignalCount(candidateSignals, candidateDomain) >= 2 && getDomainSignalCount(jobSignals, jobDomain) >= 2;
	const bool strongFamilyEvidence = getDomainSignalCount(candidateFamilySignals, candidateFamily) >= 2 && getDomainSignalCount(jobFamilySignals, jobFamily) >= 2;
	const bool anyRelationshipLane = sameFamily || adjacentFamilies || relatedFamilies || adjacentDomains || relatedDomains;
	if (coverage < 0.35f)
	{
		if (sameFamily && strongFamilyEvidence) return 2.0f;
		if (adjacentFamilies && strongFamilyEvidence) return 1.4f;
		if (adjacentDomains && strongDomainEvidence) return 1.0f;
		if (relatedFamilies && strongFamilyEvidence) return 0.75f;
		if (relatedDomains && strongDomainEvidence) return 0.75f;
		return 0.0f;
	}
	if (!candidateDomain.empty() && !jobDomain.empty() && candidateDomain != jobDomain && relationship == DomainRelationship::None) return 0.0f;

	if (sameFamily && strongFamilyEvidence) return getCoverageLaneScore(coverage, 0.35f, 0.65f, 4.0f);
	if (!anyRelationshipLane) return getCoverageLaneScore(coverage, 0.5f, 0.75f, 4.0f);
	if (adjacentFamilies && strongFamilyEvidence) return getCoverageLaneScore(coverage, 0.3f, 0.52f, 2.5f);
	if (adjacentDomains && strongDomainEvidence) return getCoverageLaneScore(coverage, 0.35f, 0.6f, 2.5f);
	if (relatedDomains && strongDomainEvidence) return getCoverageLaneScore(coverage, 0.36f, 0.62f, 2.0f);
	if (relatedFamilies && strongFamilyEvidence) return getCoverageLaneScore(coverage, 0.34f, 0.58f, 1.75f);
	return 0.0f;
}

// Calculate a recommended job score for a candidate profile and a job card by summing the scores from various criteria such as domain alignment, field alignment, skill match, major relevance, work mode fit, location fit, education fit, experience fit, and salary fit. This allows for an overall scoring of how well a candidate matches a job description based on multiple relevant factors.
float ScoringLogic::GetRecommendedJobScore(const S_CandidateProfile& candidate, const S_JobCard& job)
{
	return getDomainAlignmentScore(candidate, job)
		+ GetFieldAlignmentScore(candidate, job)
		+ GetSkillMatchScore(candidate, job)
		+ GetMajorRelevanceScore(candidate, job)
		+ getWorkModeScore(candidate, job)
		+ getLocationScore(candidate, job)
		+ getEducationScore(candidate, job)
		+ getExperienceScore(candidate, job)
		+ getSalaryFitScore(candidate, job);
}

// Convert a raw score to a match percentage by normalizing the score within a defined range and applying a non-linear transformation to boost higher scores. This allows for a more intuitive representation of how well a candidate matches a job description, with scores above 65% receiving an additional boost to reflect stronger matches.
int ScoringLogic::ToMatchPercentage(float score)
{
	constexpr float maxRawScore = 100.0f;
	constexpr float minRawScore = -55.0f;
	float normalized = std::clamp(((score - minRawScore) / (maxRawScore - minRawScore)) * 100.0f, 0.0f, 100.0f);
	if (normalized > 65.0f)
	{
		normalized = std::min(100.0f, 65.0f + ((normalized - 65.0f) * 1.45f));
	}
	return static_cast<int>(std::lround(normalized));
}

// Evaluate a candidate's match to a job description by calculating a raw score based on various criteria and converting it to a match percentage, without providing detailed match reasons. This allows for a quick assessment of how well a candidate matches a job description based on the overall score, which can be useful for sorting and filtering candidates in search results.
ScoringLogic::ScoreOnlyEvaluation ScoringLogic::EvaluateCandidateJobScoreOnly(const S_CandidateProfile& candidate, const S_JobCard& job)
{
	ScoreOnlyEvaluation evaluation;
	evaluation.rawScore = GetRecommendedJobScore(candidate, job);
	evaluation.matchScore = ToMatchPercentage(evaluation.rawScore);
	return evaluation;
}

// Evaluate a candidate's match to a job description by calculating a raw score, converting it to a match percentage, and providing detailed match reasons for the candidate audience. This allows for a comprehensive assessment of how well a candidate matches a job description, with specific feedback on the strengths and weaknesses of the match from the candidate's perspective.
ScoringLogic::MatchEvaluation ScoringLogic::EvaluateCandidateJobForCandidate(const S_CandidateProfile& candidate, const S_JobCard& job)
{
	MatchEvaluation evaluation;
	const auto scoreOnly = EvaluateCandidateJobScoreOnly(candidate, job);
	evaluation.rawScore = scoreOnly.rawScore;
	evaluation.matchScore = scoreOnly.matchScore;
	evaluation.matchReasons = BuildCandidateMatchReasons(candidate, job);
	return evaluation;
}

// Evaluate a candidate's match to a job description by calculating a raw score, converting it to a match percentage, and providing detailed match reasons for the employer audience. This allows for a comprehensive assessment of how well a candidate matches a job description, with specific feedback on the strengths and weaknesses of the match from the employer's perspective.
ScoringLogic::MatchEvaluation ScoringLogic::EvaluateCandidateJobForEmployer(const S_CandidateProfile& candidate, const S_JobCard& job)
{
	MatchEvaluation evaluation;
	const auto scoreOnly = EvaluateCandidateJobScoreOnly(candidate, job);
	evaluation.rawScore = scoreOnly.rawScore;
	evaluation.matchScore = scoreOnly.matchScore;
	evaluation.matchReasons = BuildEmployerMatchReasons(candidate, job);
	return evaluation;
}

std::vector<std::string> ScoringLogic::BuildCandidateMatchReasons(const S_CandidateProfile& candidate, const S_JobCard& job)
{
	return buildMatchReasons(candidate, job, MatchReasonAudience::Candidate);
}

std::vector<std::string> ScoringLogic::BuildEmployerMatchReasons(const S_CandidateProfile& candidate, const S_JobCard& job)
{
	return buildMatchReasons(candidate, job, MatchReasonAudience::Employer);
}
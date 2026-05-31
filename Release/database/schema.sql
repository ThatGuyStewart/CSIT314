-- ============================================================
-- Intelligent Talent Matching Platform - PostgreSQL Schema
-- CSIT314 Group Project
-- ============================================================

-- Database creation should be handled by the application.
-- This file should only contain schema objects for the current database.

-- ============================================================
-- UPDATED_AT TRIGGER SUPPORT
-- ============================================================

CREATE OR REPLACE FUNCTION set_updated_at()
RETURNS TRIGGER AS
$$
BEGIN
    NEW.updated_at = CURRENT_TIMESTAMP;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

-- ============================================================
-- USERS TABLE
-- ============================================================

CREATE TABLE IF NOT EXISTS users (
    id BIGSERIAL PRIMARY KEY,
    full_name VARCHAR(150) NOT NULL,
    email VARCHAR(191) NOT NULL UNIQUE,
    password_hash VARCHAR(255) NOT NULL,
    role VARCHAR(20) NOT NULL DEFAULT 'candidate',
    membership_status VARCHAR(20) NOT NULL DEFAULT 'free',
    created_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT chk_users_role
        CHECK (role IN ('candidate', 'employer', 'admin')),
    CONSTRAINT chk_users_membership_status
        CHECK (membership_status IN ('free', 'premium'))
);

CREATE INDEX IF NOT EXISTS idx_users_email ON users(email);
CREATE INDEX IF NOT EXISTS idx_users_role ON users(role);

DROP TRIGGER IF EXISTS trg_users_updated_at ON users;
CREATE TRIGGER trg_users_updated_at
BEFORE UPDATE ON users
FOR EACH ROW
EXECUTE FUNCTION set_updated_at();

-- ============================================================
-- CANDIDATE PROFILES TABLE
-- ============================================================

CREATE TABLE IF NOT EXISTS candidate_profiles (
    id BIGSERIAL PRIMARY KEY,
    user_id BIGINT NOT NULL UNIQUE,
    contact_info VARCHAR(255),
    education VARCHAR(20) DEFAULT 'Bachelor''s',
    major VARCHAR(150),
    years_experience INT NOT NULL DEFAULT 0,
    work_experience TEXT,
    skills TEXT,
    preferred_work_mode VARCHAR(20) DEFAULT 'Hybrid',
    preferred_location VARCHAR(150),
    expected_salary NUMERIC(10,2),
    employment_type VARCHAR(20) DEFAULT 'Full-time',
    summary TEXT,
    portfolio_url VARCHAR(255),
    created_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT fk_candidate_profiles_user
        FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    CONSTRAINT chk_candidate_profiles_education
        CHECK (education IN ('High School', 'Bachelor''s', 'Master''s', 'PhD', 'Other')),
    CONSTRAINT chk_candidate_profiles_preferred_work_mode
        CHECK (preferred_work_mode IN ('Remote', 'On-site', 'Hybrid')),
    CONSTRAINT chk_candidate_profiles_employment_type
        CHECK (employment_type IN ('Full-time', 'Part-time', 'Internship', 'Contract')),
    CONSTRAINT chk_candidate_profiles_years_experience
        CHECK (years_experience >= 0),
    CONSTRAINT chk_candidate_profiles_expected_salary
        CHECK (expected_salary IS NULL OR expected_salary >= 0)
);

CREATE INDEX IF NOT EXISTS idx_candidate_profiles_user_id ON candidate_profiles(user_id);

DROP TRIGGER IF EXISTS trg_candidate_profiles_updated_at ON candidate_profiles;
CREATE TRIGGER trg_candidate_profiles_updated_at
BEFORE UPDATE ON candidate_profiles
FOR EACH ROW
EXECUTE FUNCTION set_updated_at();

-- ============================================================
-- COMPANY PROFILES TABLE
-- ============================================================

CREATE TABLE IF NOT EXISTS company_profiles (
    id BIGSERIAL PRIMARY KEY,
    user_id BIGINT NOT NULL UNIQUE,
    company_name VARCHAR(200) NOT NULL,
    company_email VARCHAR(191),
    company_phone VARCHAR(50),
    industry VARCHAR(20) DEFAULT 'Technology',
    company_size VARCHAR(20) DEFAULT '11-50',
    company_location VARCHAR(150),
    company_description TEXT,
    website_url VARCHAR(255),
    created_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT fk_company_profiles_user
        FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    CONSTRAINT chk_company_profiles_industry
        CHECK (industry IN ('Technology', 'Finance', 'Healthcare', 'Education', 'Retail', 'Manufacturing', 'Media', 'Other')),
    CONSTRAINT chk_company_profiles_company_size
        CHECK (company_size IN ('1-10', '11-50', '51-200', '201-500', '500+'))
);

CREATE INDEX IF NOT EXISTS idx_company_profiles_user_id ON company_profiles(user_id);

DROP TRIGGER IF EXISTS trg_company_profiles_updated_at ON company_profiles;
CREATE TRIGGER trg_company_profiles_updated_at
BEFORE UPDATE ON company_profiles
FOR EACH ROW
EXECUTE FUNCTION set_updated_at();

-- ============================================================
-- JOB POSTINGS TABLE
-- ============================================================

CREATE TABLE IF NOT EXISTS job_postings (
    id BIGSERIAL PRIMARY KEY,
    employer_id BIGINT NOT NULL,
    company_info VARCHAR(200),
    job_title VARCHAR(200) NOT NULL,
    job_description TEXT,
    required_education VARCHAR(20) DEFAULT 'Any',
    required_skills TEXT,
    required_experience INT NOT NULL DEFAULT 0,
    work_mode VARCHAR(20) DEFAULT 'On-site',
    job_location VARCHAR(150),
    salary_min NUMERIC(10,2),
    salary_max NUMERIC(10,2),
    job_type VARCHAR(20) DEFAULT 'Full-time',
    career_level VARCHAR(20) DEFAULT 'Entry-level',
    application_deadline DATE,
    status VARCHAR(20) DEFAULT 'Open',
    created_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT fk_job_postings_employer
        FOREIGN KEY (employer_id) REFERENCES users(id) ON DELETE CASCADE,
    CONSTRAINT chk_job_postings_required_education
        CHECK (required_education IN ('High School', 'Bachelor''s', 'Master''s', 'PhD', 'Any')),
    CONSTRAINT chk_job_postings_work_mode
        CHECK (work_mode IN ('Remote', 'On-site', 'Hybrid')),
    CONSTRAINT chk_job_postings_job_type
        CHECK (job_type IN ('Full-time', 'Part-time', 'Internship', 'Contract')),
    CONSTRAINT chk_job_postings_career_level
        CHECK (career_level IN ('Entry-level', 'Mid-level', 'Senior')),
    CONSTRAINT chk_job_postings_status
        CHECK (status IN ('Open', 'Closed')),
    CONSTRAINT chk_job_postings_required_experience
        CHECK (required_experience >= 0),
    CONSTRAINT chk_job_postings_salary_min
        CHECK (salary_min IS NULL OR salary_min >= 0),
    CONSTRAINT chk_job_postings_salary_max
        CHECK (salary_max IS NULL OR salary_max >= 0),
    CONSTRAINT chk_job_postings_salary_range
        CHECK (
            salary_min IS NULL
            OR salary_max IS NULL
            OR salary_min <= salary_max
        )
);

CREATE INDEX IF NOT EXISTS idx_job_postings_employer_id ON job_postings(employer_id);
CREATE INDEX IF NOT EXISTS idx_job_postings_status ON job_postings(status);
CREATE INDEX IF NOT EXISTS idx_job_postings_work_mode ON job_postings(work_mode);

DROP TRIGGER IF EXISTS trg_job_postings_updated_at ON job_postings;
CREATE TRIGGER trg_job_postings_updated_at
BEFORE UPDATE ON job_postings
FOR EACH ROW
EXECUTE FUNCTION set_updated_at();

-- ============================================================
-- SEED DATA
-- ============================================================

INSERT INTO users (full_name, email, password_hash, role, membership_status)
VALUES
    ('Platform Admin', 'admin@platform.com', 'Password123', 'admin', 'premium'),
    ('Alice Johnson', 'alice@example.com', 'Password123', 'candidate', 'free'),
    ('TechCorp Pty Ltd', 'techcorp@example.com', 'Password123', 'employer', 'premium'),
    ('Brian Lee', 'brian@example.com', 'Password123', 'candidate', 'premium'),
    ('Chloe Martin', 'chloe@example.com', 'Password123', 'candidate', 'free'),
    ('Daniel Kim', 'daniel@example.com', 'Password123', 'candidate', 'free'),
    ('DataWorks Pty Ltd', 'dataworks@example.com', 'Password123', 'employer', 'premium'),
    ('HealthBridge Pty Ltd', 'healthbridge@example.com', 'Password123', 'employer', 'free'),
    ('FinEdge Pty Ltd', 'finedge@example.com', 'Password123', 'employer', 'premium'),
    ('EduSpark Pty Ltd', 'eduspark@example.com', 'Password123', 'employer', 'free'),
    ('BuildSmart Pty Ltd', 'buildsmart@example.com', 'Password123', 'employer', 'premium'),
    ('Mia Chen', 'mia@example.com', 'Password123', 'candidate', 'premium'),
    ('Noah Patel', 'noah@example.com', 'Password123', 'candidate', 'free'),
    ('Olivia Brown', 'olivia@example.com', 'Password123', 'candidate', 'premium'),
    ('Ethan Walker', 'ethan@example.com', 'Password123', 'candidate', 'free'),
    ('Sophia Nguyen', 'sophia@example.com', 'Password123', 'candidate', 'premium'),
    ('Liam Scott', 'liam@example.com', 'Password123', 'candidate', 'free')
ON CONFLICT (email) DO NOTHING;

INSERT INTO candidate_profiles
(
    user_id,
    contact_info,
    education,
    major,
    years_experience,
    work_experience,
    skills,
    preferred_work_mode,
    preferred_location,
    expected_salary,
    employment_type,
    summary,
    portfolio_url
)
SELECT
    u.id,
    'alice@example.com | 0400 111 222',
    'Bachelor''s',
    'Computer Science',
    3,
    'Frontend developer at Startup Labs; internship at BlueWave Digital.',
    'JavaScript, TypeScript, React, HTML, CSS, Node.js',
    'Hybrid',
    'Sydney',
    85000.00,
    'Full-time',
    'Frontend-focused software developer with strong UI and JavaScript skills.',
    'https://portfolio.example.com/alice'
FROM users u
WHERE u.email = 'alice@example.com'
ON CONFLICT (user_id) DO NOTHING;

INSERT INTO candidate_profiles
(
    user_id,
    contact_info,
    education,
    major,
    years_experience,
    work_experience,
    skills,
    preferred_work_mode,
    preferred_location,
    expected_salary,
    employment_type,
    summary,
    portfolio_url
)
SELECT
    u.id,
    'brian@example.com | 0400 222 333',
    'Master''s',
    'Software Engineering',
    5,
    'Backend engineer at FinStack; software engineer at CloudArc.',
    'C++, C#, PostgreSQL, Docker, REST APIs, Azure',
    'Remote',
    'Melbourne',
    115000.00,
    'Full-time',
    'Backend-focused engineer with cloud and database experience.',
    'https://portfolio.example.com/brian'
FROM users u
WHERE u.email = 'brian@example.com'
ON CONFLICT (user_id) DO NOTHING;

INSERT INTO candidate_profiles
(
    user_id,
    contact_info,
    education,
    major,
    years_experience,
    work_experience,
    skills,
    preferred_work_mode,
    preferred_location,
    expected_salary,
    employment_type,
    summary,
    portfolio_url
)
SELECT
    u.id,
    'chloe@example.com | 0400 333 444',
    'Bachelor''s',
    'Data Science',
    2,
    'Data analyst at Insight Labs; internship at City Analytics.',
    'Python, SQL, Power BI, Pandas, Machine Learning, Excel',
    'Hybrid',
    'Brisbane',
    82000.00,
    'Full-time',
    'Data-driven analyst with reporting and ML project experience.',
    'https://portfolio.example.com/chloe'
FROM users u
WHERE u.email = 'chloe@example.com'
ON CONFLICT (user_id) DO NOTHING;

INSERT INTO candidate_profiles
(
    user_id,
    contact_info,
    education,
    major,
    years_experience,
    work_experience,
    skills,
    preferred_work_mode,
    preferred_location,
    expected_salary,
    employment_type,
    summary,
    portfolio_url
)
SELECT
    u.id,
    'daniel@example.com | 0400 444 555',
    'Bachelor''s',
    'Information Systems',
    4,
    'Full stack developer at RetailFlow; web developer at BrightApps.',
    'JavaScript, TypeScript, React, Node.js, PostgreSQL, AWS',
    'On-site',
    'Sydney',
    98000.00,
    'Full-time',
    'Full stack developer experienced in product delivery and cloud deployment.',
    'https://portfolio.example.com/daniel'
FROM users u
WHERE u.email = 'daniel@example.com'
ON CONFLICT (user_id) DO NOTHING;

INSERT INTO company_profiles
(
    user_id,
    company_name,
    company_email,
    company_phone,
    industry,
    company_size,
    company_location,
    company_description,
    website_url
)
SELECT
    u.id,
    'TechCorp',
    'techcorp@example.com',
    '+61 2 9000 1234',
    'Technology',
    '51-200',
    'Sydney',
    'TechCorp builds modern hiring and workforce platforms for Australian businesses.',
    'https://techcorp.example.com'
FROM users u
WHERE u.email = 'techcorp@example.com'
ON CONFLICT (user_id) DO NOTHING;

INSERT INTO company_profiles
(
    user_id,
    company_name,
    company_email,
    company_phone,
    industry,
    company_size,
    company_location,
    company_description,
    website_url
)
SELECT
    u.id,
    'DataWorks',
    'dataworks@example.com',
    '+61 3 9000 5678',
    'Technology',
    '11-50',
    'Melbourne',
    'DataWorks builds analytics tools and internal data platforms for scaling teams.',
    'https://dataworks.example.com'
FROM users u
WHERE u.email = 'dataworks@example.com'
ON CONFLICT (user_id) DO NOTHING;

INSERT INTO company_profiles
(
    user_id,
    company_name,
    company_email,
    company_phone,
    industry,
    company_size,
    company_location,
    company_description,
    website_url
)
SELECT
    u.id,
    'HealthBridge',
    'healthbridge@example.com',
    '+61 7 9000 2468',
    'Healthcare',
    '51-200',
    'Brisbane',
    'HealthBridge delivers digital healthcare products and patient-facing systems.',
    'https://healthbridge.example.com'
FROM users u
WHERE u.email = 'healthbridge@example.com'
ON CONFLICT (user_id) DO NOTHING;

INSERT INTO company_profiles
(
    user_id,
    company_name,
    company_email,
    company_phone,
    industry,
    company_size,
    company_location,
    company_description,
    website_url
)
SELECT
    u.id,
    'FinEdge',
    'finedge@example.com',
    '+61 2 9100 7788',
    'Finance',
    '201-500',
    'Sydney',
    'FinEdge develops digital banking platforms, fraud analytics, and compliance tooling.',
    'https://finedge.example.com'
FROM users u
WHERE u.email = 'finedge@example.com'
ON CONFLICT (user_id) DO NOTHING;

INSERT INTO company_profiles
(
    user_id,
    company_name,
    company_email,
    company_phone,
    industry,
    company_size,
    company_location,
    company_description,
    website_url
)
SELECT
    u.id,
    'EduSpark',
    'eduspark@example.com',
    '+61 3 9200 4455',
    'Education',
    '11-50',
    'Adelaide',
    'EduSpark builds learning platforms, virtual classrooms, and curriculum analytics products.',
    'https://eduspark.example.com'
FROM users u
WHERE u.email = 'eduspark@example.com'
ON CONFLICT (user_id) DO NOTHING;

INSERT INTO company_profiles
(
    user_id,
    company_name,
    company_email,
    company_phone,
    industry,
    company_size,
    company_location,
    company_description,
    website_url
)
SELECT
    u.id,
    'BuildSmart',
    'buildsmart@example.com',
    '+61 7 9300 1122',
    'Manufacturing',
    '51-200',
    'Perth',
    'BuildSmart creates industrial automation, IoT monitoring, and maintenance planning systems.',
    'https://buildsmart.example.com'
FROM users u
WHERE u.email = 'buildsmart@example.com'
ON CONFLICT (user_id) DO NOTHING;

INSERT INTO candidate_profiles
(
    user_id,
    contact_info,
    education,
    major,
    years_experience,
    work_experience,
    skills,
    preferred_work_mode,
    preferred_location,
    expected_salary,
    employment_type,
    summary,
    portfolio_url
)
SELECT
    u.id,
    'mia@example.com | 0400 555 101',
    'Master''s',
    'Finance and Analytics',
    6,
    'Senior financial analyst at CapitalNorth; BI analyst at LedgerPoint.',
    'SQL, Power BI, Tableau, Financial Modelling, Excel, Risk Analysis',
    'Hybrid',
    'Sydney',
    125000.00,
    'Full-time',
    'Finance analytics professional with strong reporting, forecasting, and stakeholder communication skills.',
    'https://portfolio.example.com/mia'
FROM users u
WHERE u.email = 'mia@example.com'
ON CONFLICT (user_id) DO NOTHING;

INSERT INTO candidate_profiles
(
    user_id,
    contact_info,
    education,
    major,
    years_experience,
    work_experience,
    skills,
    preferred_work_mode,
    preferred_location,
    expected_salary,
    employment_type,
    summary,
    portfolio_url
)
SELECT
    u.id,
    'noah@example.com | 0400 555 202',
    'Bachelor''s',
    'Cybersecurity',
    4,
    'Security analyst at SecureGrid; SOC engineer at NetShield.',
    'SIEM, Splunk, Incident Response, Network Security, Python, Vulnerability Management',
    'Remote',
    'Melbourne',
    112000.00,
    'Full-time',
    'Cybersecurity specialist focused on monitoring, incident response, and secure operations.',
    'https://portfolio.example.com/noah'
FROM users u
WHERE u.email = 'noah@example.com'
ON CONFLICT (user_id) DO NOTHING;

INSERT INTO candidate_profiles
(
    user_id,
    contact_info,
    education,
    major,
    years_experience,
    work_experience,
    skills,
    preferred_work_mode,
    preferred_location,
    expected_salary,
    employment_type,
    summary,
    portfolio_url
)
SELECT
    u.id,
    'olivia@example.com | 0400 555 303',
    'Bachelor''s',
    'UX Design',
    5,
    'Product designer at LearnLoop; UX designer at Studio Pixel.',
    'Figma, User Research, Prototyping, Design Systems, Accessibility, Wireframing',
    'Hybrid',
    'Adelaide',
    98000.00,
    'Full-time',
    'UX designer with strengths in product discovery, accessibility, and collaborative design systems.',
    'https://portfolio.example.com/olivia'
FROM users u
WHERE u.email = 'olivia@example.com'
ON CONFLICT (user_id) DO NOTHING;

INSERT INTO candidate_profiles
(
    user_id,
    contact_info,
    education,
    major,
    years_experience,
    work_experience,
    skills,
    preferred_work_mode,
    preferred_location,
    expected_salary,
    employment_type,
    summary,
    portfolio_url
)
SELECT
    u.id,
    'ethan@example.com | 0400 555 404',
    'Bachelor''s',
    'Mechanical Engineering',
    7,
    'Automation engineer at PlantCore; maintenance engineer at Allied Manufacturing.',
    'PLC, SCADA, AutoCAD, Preventive Maintenance, IoT Sensors, Root Cause Analysis',
    'On-site',
    'Perth',
    118000.00,
    'Full-time',
    'Industrial automation engineer experienced in plant reliability and manufacturing operations.',
    'https://portfolio.example.com/ethan'
FROM users u
WHERE u.email = 'ethan@example.com'
ON CONFLICT (user_id) DO NOTHING;

INSERT INTO candidate_profiles
(
    user_id,
    contact_info,
    education,
    major,
    years_experience,
    work_experience,
    skills,
    preferred_work_mode,
    preferred_location,
    expected_salary,
    employment_type,
    summary,
    portfolio_url
)
SELECT
    u.id,
    'sophia@example.com | 0400 555 505',
    'Master''s',
    'Healthcare Informatics',
    3,
    'Clinical systems analyst at MedAxis; implementation consultant at CareFlow.',
    'HL7, SQL, Healthcare Systems, Requirements Gathering, Data Mapping, UAT',
    'Hybrid',
    'Brisbane',
    97000.00,
    'Full-time',
    'Healthcare technology analyst experienced in clinical workflows and system integrations.',
    'https://portfolio.example.com/sophia'
FROM users u
WHERE u.email = 'sophia@example.com'
ON CONFLICT (user_id) DO NOTHING;

INSERT INTO candidate_profiles
(
    user_id,
    contact_info,
    education,
    major,
    years_experience,
    work_experience,
    skills,
    preferred_work_mode,
    preferred_location,
    expected_salary,
    employment_type,
    summary,
    portfolio_url
)
SELECT
    u.id,
    'liam@example.com | 0400 555 606',
    'Bachelor''s',
    'Digital Marketing',
    4,
    'Growth marketer at BrightReach; content strategist at Wave Media.',
    'SEO, Google Analytics, Content Strategy, Paid Search, Email Marketing, CRM',
    'Remote',
    'Gold Coast',
    88000.00,
    'Full-time',
    'Digital marketer focused on acquisition, campaign analytics, and conversion optimization.',
    'https://portfolio.example.com/liam'
FROM users u
WHERE u.email = 'liam@example.com'
ON CONFLICT (user_id) DO NOTHING;

INSERT INTO job_postings
(
    employer_id,
    company_info,
    job_title,
    job_description,
    required_education,
    required_skills,
    required_experience,
    work_mode,
    job_location,
    salary_min,
    salary_max,
    job_type,
    career_level,
    application_deadline,
    status
)
SELECT
    u.id,
    'TechCorp - Sydney',
    'Frontend Developer',
    'Build responsive UI features for the TalentMatch platform.',
    'Bachelor''s',
    'JavaScript, TypeScript, React, HTML, CSS',
    2,
    'Hybrid',
    'Sydney',
    80000.00,
    100000.00,
    'Full-time',
    'Mid-level',
    (CURRENT_DATE + INTERVAL '30 days')::date,
    'Open'
FROM users u
WHERE u.email = 'techcorp@example.com'
  AND NOT EXISTS
  (
      SELECT 1
      FROM job_postings jp
      WHERE jp.employer_id = u.id
        AND jp.job_title = 'Frontend Developer'
  );

INSERT INTO job_postings
(
    employer_id,
    company_info,
    job_title,
    job_description,
    required_education,
    required_skills,
    required_experience,
    work_mode,
    job_location,
    salary_min,
    salary_max,
    job_type,
    career_level,
    application_deadline,
    status
)
SELECT
    u.id,
    'TechCorp - Sydney',
    'Backend Developer',
    'Design APIs and PostgreSQL-backed services for the TalentMatch platform.',
    'Bachelor''s',
    'C++, PostgreSQL, REST APIs, SQL',
    3,
    'On-site',
    'Sydney',
    90000.00,
    115000.00,
    'Full-time',
    'Mid-level',
    (CURRENT_DATE + INTERVAL '45 days')::date,
    'Open'
FROM users u
WHERE u.email = 'techcorp@example.com'
  AND NOT EXISTS
  (
      SELECT 1
      FROM job_postings jp
      WHERE jp.employer_id = u.id
        AND jp.job_title = 'Backend Developer'
  );

INSERT INTO job_postings
(
    employer_id,
    company_info,
    job_title,
    job_description,
    required_education,
    required_skills,
    required_experience,
    work_mode,
    job_location,
    salary_min,
    salary_max,
    job_type,
    career_level,
    application_deadline,
    status
)
SELECT
    u.id,
    'DataWorks - Melbourne',
    'Data Analyst',
    'Build dashboards, analyze datasets, and support reporting for internal and client-facing analytics products.',
    'Bachelor''s',
    'Python, SQL, Power BI, Excel, Pandas',
    2,
    'Hybrid',
    'Melbourne',
    85000.00,
    105000.00,
    'Full-time',
    'Mid-level',
    (CURRENT_DATE + INTERVAL '35 days')::date,
    'Open'
FROM users u
WHERE u.email = 'dataworks@example.com'
  AND NOT EXISTS
  (
      SELECT 1
      FROM job_postings jp
      WHERE jp.employer_id = u.id
        AND jp.job_title = 'Data Analyst'
  );

INSERT INTO job_postings
(
    employer_id,
    company_info,
    job_title,
    job_description,
    required_education,
    required_skills,
    required_experience,
    work_mode,
    job_location,
    salary_min,
    salary_max,
    job_type,
    career_level,
    application_deadline,
    status
)
SELECT
    u.id,
    'HealthBridge - Brisbane',
    'Full Stack Developer',
    'Develop patient-facing web features and backend integrations for digital healthcare platforms.',
    'Bachelor''s',
    'JavaScript, TypeScript, React, Node.js, PostgreSQL',
    3,
    'On-site',
    'Brisbane',
    95000.00,
    120000.00,
    'Full-time',
    'Mid-level',
    (CURRENT_DATE + INTERVAL '40 days')::date,
    'Open'
FROM users u
WHERE u.email = 'healthbridge@example.com'
  AND NOT EXISTS
  (
      SELECT 1
      FROM job_postings jp
      WHERE jp.employer_id = u.id
        AND jp.job_title = 'Full Stack Developer'
  );

INSERT INTO job_postings
(
    employer_id,
    company_info,
    job_title,
    job_description,
    required_education,
    required_skills,
    required_experience,
    work_mode,
    job_location,
    salary_min,
    salary_max,
    job_type,
    career_level,
    application_deadline,
    status
)
SELECT
    u.id,
    'DataWorks - Melbourne',
    'Cloud Engineer',
    'Maintain cloud infrastructure, deployment pipelines, and platform reliability for analytics services.',
    'Bachelor''s',
    'AWS, Docker, CI/CD, PostgreSQL, Linux',
    4,
    'Remote',
    'Melbourne',
    110000.00,
    135000.00,
    'Full-time',
    'Senior',
    (CURRENT_DATE + INTERVAL '50 days')::date,
    'Open'
FROM users u
WHERE u.email = 'dataworks@example.com'
  AND NOT EXISTS
  (
      SELECT 1
      FROM job_postings jp
      WHERE jp.employer_id = u.id
        AND jp.job_title = 'Cloud Engineer'
  );

INSERT INTO job_postings
(
    employer_id,
    company_info,
    job_title,
    job_description,
    required_education,
    required_skills,
    required_experience,
    work_mode,
    job_location,
    salary_min,
    salary_max,
    job_type,
    career_level,
    application_deadline,
    status
)
SELECT
    u.id,
    'FinEdge - Sydney',
    'Fraud Analytics Specialist',
    'Build fraud monitoring dashboards, investigate anomalies, and improve transaction risk models.',
    'Bachelor''s',
    'SQL, Python, Risk Analysis, Tableau, Fraud Detection, Statistics',
    4,
    'Hybrid',
    'Sydney',
    105000.00,
    128000.00,
    'Full-time',
    'Mid-level',
    (CURRENT_DATE + INTERVAL '32 days')::date,
    'Open'
FROM users u
WHERE u.email = 'finedge@example.com'
  AND NOT EXISTS
  (
      SELECT 1
      FROM job_postings jp
      WHERE jp.employer_id = u.id
        AND jp.job_title = 'Fraud Analytics Specialist'
  );

INSERT INTO job_postings
(
    employer_id,
    company_info,
    job_title,
    job_description,
    required_education,
    required_skills,
    required_experience,
    work_mode,
    job_location,
    salary_min,
    salary_max,
    job_type,
    career_level,
    application_deadline,
    status
)
SELECT
    u.id,
    'FinEdge - Sydney',
    'Cybersecurity Analyst',
    'Monitor threats, review security events, and strengthen controls across banking platforms.',
    'Bachelor''s',
    'SIEM, Splunk, Incident Response, Python, Network Security, IAM',
    3,
    'Remote',
    'Sydney',
    110000.00,
    132000.00,
    'Full-time',
    'Mid-level',
    (CURRENT_DATE + INTERVAL '38 days')::date,
    'Open'
FROM users u
WHERE u.email = 'finedge@example.com'
  AND NOT EXISTS
  (
      SELECT 1
      FROM job_postings jp
      WHERE jp.employer_id = u.id
        AND jp.job_title = 'Cybersecurity Analyst'
  );

INSERT INTO job_postings
(
    employer_id,
    company_info,
    job_title,
    job_description,
    required_education,
    required_skills,
    required_experience,
    work_mode,
    job_location,
    salary_min,
    salary_max,
    job_type,
    career_level,
    application_deadline,
    status
)
SELECT
    u.id,
    'EduSpark - Adelaide',
    'UX Designer',
    'Design intuitive student and teacher workflows for a modern learning platform.',
    'Bachelor''s',
    'Figma, User Research, Prototyping, Accessibility, Design Systems, Wireframing',
    3,
    'Hybrid',
    'Adelaide',
    90000.00,
    108000.00,
    'Full-time',
    'Mid-level',
    (CURRENT_DATE + INTERVAL '28 days')::date,
    'Open'
FROM users u
WHERE u.email = 'eduspark@example.com'
  AND NOT EXISTS
  (
      SELECT 1
      FROM job_postings jp
      WHERE jp.employer_id = u.id
        AND jp.job_title = 'UX Designer'
  );

INSERT INTO job_postings
(
    employer_id,
    company_info,
    job_title,
    job_description,
    required_education,
    required_skills,
    required_experience,
    work_mode,
    job_location,
    salary_min,
    salary_max,
    job_type,
    career_level,
    application_deadline,
    status
)
SELECT
    u.id,
    'EduSpark - Adelaide',
    'Learning Platform Product Manager',
    'Coordinate roadmap delivery, gather education-sector requirements, and align cross-functional teams.',
    'Bachelor''s',
    'Product Management, Stakeholder Management, Agile, User Stories, Analytics, Roadmapping',
    5,
    'Hybrid',
    'Adelaide',
    115000.00,
    140000.00,
    'Full-time',
    'Senior',
    (CURRENT_DATE + INTERVAL '44 days')::date,
    'Open'
FROM users u
WHERE u.email = 'eduspark@example.com'
  AND NOT EXISTS
  (
      SELECT 1
      FROM job_postings jp
      WHERE jp.employer_id = u.id
        AND jp.job_title = 'Learning Platform Product Manager'
  );

INSERT INTO job_postings
(
    employer_id,
    company_info,
    job_title,
    job_description,
    required_education,
    required_skills,
    required_experience,
    work_mode,
    job_location,
    salary_min,
    salary_max,
    job_type,
    career_level,
    application_deadline,
    status
)
SELECT
    u.id,
    'BuildSmart - Perth',
    'Industrial Automation Engineer',
    'Design and maintain PLC-driven automation workflows for factory monitoring and throughput optimization.',
    'Bachelor''s',
    'PLC, SCADA, AutoCAD, IoT Sensors, Preventive Maintenance, Troubleshooting',
    5,
    'On-site',
    'Perth',
    118000.00,
    145000.00,
    'Full-time',
    'Senior',
    (CURRENT_DATE + INTERVAL '36 days')::date,
    'Open'
FROM users u
WHERE u.email = 'buildsmart@example.com'
  AND NOT EXISTS
  (
      SELECT 1
      FROM job_postings jp
      WHERE jp.employer_id = u.id
        AND jp.job_title = 'Industrial Automation Engineer'
  );

INSERT INTO job_postings
(
    employer_id,
    company_info,
    job_title,
    job_description,
    required_education,
    required_skills,
    required_experience,
    work_mode,
    job_location,
    salary_min,
    salary_max,
    job_type,
    career_level,
    application_deadline,
    status
)
SELECT
    u.id,
    'HealthBridge - Brisbane',
    'Healthcare Systems Analyst',
    'Support healthcare product implementations, data mapping, and workflow improvements with clinical teams.',
    'Bachelor''s',
    'HL7, SQL, Requirements Gathering, Data Mapping, UAT, Healthcare Systems',
    2,
    'Hybrid',
    'Brisbane',
    92000.00,
    108000.00,
    'Full-time',
    'Entry-level',
    (CURRENT_DATE + INTERVAL '34 days')::date,
    'Open'
FROM users u
WHERE u.email = 'healthbridge@example.com'
  AND NOT EXISTS
  (
      SELECT 1
      FROM job_postings jp
      WHERE jp.employer_id = u.id
        AND jp.job_title = 'Healthcare Systems Analyst'
  );

INSERT INTO job_postings
(
    employer_id,
    company_info,
    job_title,
    job_description,
    required_education,
    required_skills,
    required_experience,
    work_mode,
    job_location,
    salary_min,
    salary_max,
    job_type,
    career_level,
    application_deadline,
    status
)
SELECT
    u.id,
    'BuildSmart - Perth',
    'Digital Marketing Specialist',
    'Lead industrial product campaigns, optimize digital acquisition, and improve lead generation reporting.',
    'Bachelor''s',
    'SEO, Google Analytics, Paid Search, Content Strategy, CRM, Email Marketing',
    3,
    'Remote',
    'Perth',
    82000.00,
    98000.00,
    'Full-time',
    'Mid-level',
    (CURRENT_DATE + INTERVAL '26 days')::date,
    'Open'
FROM users u
WHERE u.email = 'buildsmart@example.com'
  AND NOT EXISTS
  (
      SELECT 1
      FROM job_postings jp
      WHERE jp.employer_id = u.id
        AND jp.job_title = 'Digital Marketing Specialist'
  );

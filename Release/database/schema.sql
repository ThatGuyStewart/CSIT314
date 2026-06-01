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
-- CANDIDATE APPLICATIONS TABLE
-- ============================================================

CREATE TABLE IF NOT EXISTS candidate_applications (
    id BIGSERIAL PRIMARY KEY,
    candidate_id BIGINT NOT NULL,
    job_posting_id BIGINT NOT NULL,
    created_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT fk_candidate_applications_candidate
        FOREIGN KEY (candidate_id) REFERENCES users(id) ON DELETE CASCADE,
    CONSTRAINT fk_candidate_applications_job
        FOREIGN KEY (job_posting_id) REFERENCES job_postings(id) ON DELETE CASCADE,
    CONSTRAINT uq_candidate_applications_candidate_job
        UNIQUE (candidate_id, job_posting_id)
);

CREATE INDEX IF NOT EXISTS idx_candidate_applications_candidate_id ON candidate_applications(candidate_id);
CREATE INDEX IF NOT EXISTS idx_candidate_applications_job_posting_id ON candidate_applications(job_posting_id);

-- ============================================================
-- SEED DATA
-- ============================================================

INSERT INTO users (full_name, email, password_hash, role, membership_status)
VALUES
    ('Platform Admin', 'admin@platform.com', 'Password123', 'admin', 'premium'),
    ('Alice Johnson', 'alice@example.com', 'Password123', 'candidate', 'free'),
    ('TechCorp Pty Ltd', 'techcorp@example.com', 'Password123', 'employer', 'free'),
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

UPDATE users
SET membership_status = 'free'
WHERE email = 'techcorp@example.com'
  AND role = 'employer';

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

INSERT INTO users (full_name, email, password_hash, role, membership_status)
VALUES
    ('Priya Raman', 'priya@example.com', 'Password123', 'candidate', 'free'),
    ('Lucas Meyer', 'lucas@example.com', 'Password123', 'candidate', 'premium'),
    ('Grace O''Connor', 'grace@example.com', 'Password123', 'candidate', 'free'),
    ('Elena Petrova', 'elena@example.com', 'Password123', 'candidate', 'premium'),
    ('GreenCart Retail Pty Ltd', 'greencart@example.com', 'Password123', 'employer', 'premium'),
    ('StudioPulse Media Pty Ltd', 'studiopulse@example.com', 'Password123', 'employer', 'free'),
    ('HarborLogix Services Pty Ltd', 'harborlogix@example.com', 'Password123', 'employer', 'premium'),
    ('TerraField Environmental Pty Ltd', 'terrafield@example.com', 'Password123', 'employer', 'free')
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
    'priya@example.com | 0400 666 111',
    'Master''s',
    'Supply Chain Management',
    6,
    'Supply planner at PortLink Logistics; inventory analyst at FreshRoute Distribution.',
    'Demand Planning, Procurement, Inventory Control, ERP, Excel, Vendor Management',
    'On-site',
    'Brisbane',
    104000.00,
    'Full-time',
    'Operations-focused supply chain professional with strong planning and vendor coordination experience.',
    'https://portfolio.example.com/priya'
FROM users u
WHERE u.email = 'priya@example.com'
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
    'lucas@example.com | 0400 666 222',
    'Bachelor''s',
    'Film and Media Production',
    5,
    'Video editor at NorthFrame Studio; production coordinator at CityScreen Media.',
    'Adobe Premiere Pro, After Effects, Storyboarding, Audio Mixing, Camera Operation, Script Breakdown',
    'Hybrid',
    'Melbourne',
    93000.00,
    'Full-time',
    'Media production specialist focused on video delivery, post-production, and creative coordination.',
    'https://portfolio.example.com/lucas'
FROM users u
WHERE u.email = 'lucas@example.com'
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
    'grace@example.com | 0400 666 333',
    'Bachelor''s',
    'Retail Management',
    7,
    'Store operations lead at MarketSquare; merchandising supervisor at ValueHome Retail.',
    'Retail Operations, Merchandising, POS Systems, Inventory Audits, Team Leadership, Rostering',
    'On-site',
    'Hobart',
    88000.00,
    'Full-time',
    'Retail operations leader experienced in merchandising execution, store standards, and frontline coaching.',
    'https://portfolio.example.com/grace'
FROM users u
WHERE u.email = 'grace@example.com'
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
    'elena@example.com | 0400 666 444',
    'Master''s',
    'Environmental Science',
    4,
    'Field compliance analyst at EcoTrack; environmental coordinator at Coastal Survey Group.',
    'Environmental Auditing, GIS, Sampling Plans, Regulatory Reporting, Risk Assessments, Stakeholder Engagement',
    'Hybrid',
    'Perth',
    97000.00,
    'Full-time',
    'Environmental compliance professional with experience in monitoring programs and sustainability reporting.',
    'https://portfolio.example.com/elena'
FROM users u
WHERE u.email = 'elena@example.com'
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
    'GreenCart Retail',
    'greencart@example.com',
    '+61 3 9550 1001',
    'Retail',
    '201-500',
    'Hobart',
    'GreenCart runs multi-site retail stores and fulfillment operations with a strong focus on inventory accuracy and store execution.',
    'https://greencart.example.com'
FROM users u
WHERE u.email = 'greencart@example.com'
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
    'StudioPulse Media',
    'studiopulse@example.com',
    '+61 3 9550 1002',
    'Media',
    '11-50',
    'Melbourne',
    'StudioPulse produces commercial, digital, and broadcast media content with in-house pre-production and post-production teams.',
    'https://studiopulse.example.com'
FROM users u
WHERE u.email = 'studiopulse@example.com'
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
    'HarborLogix Services',
    'harborlogix@example.com',
    '+61 7 9550 1003',
    'Other',
    '51-200',
    'Brisbane',
    'HarborLogix manages fleet coordination, warehouse planning, and transport compliance services for industrial clients.',
    'https://harborlogix.example.com'
FROM users u
WHERE u.email = 'harborlogix@example.com'
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
    'TerraField Environmental',
    'terrafield@example.com',
    '+61 8 9550 1004',
    'Other',
    '11-50',
    'Perth',
    'TerraField delivers monitoring, compliance, and sustainability reporting services for land, water, and industrial projects.',
    'https://terrafield.example.com'
FROM users u
WHERE u.email = 'terrafield@example.com'
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
    'GreenCart Retail - Hobart',
    'Retail Operations Coordinator',
    'Coordinate stock transfers, store execution plans, roster inputs, and daily retail operations reporting across multiple sites.',
    'Bachelor''s',
    'Retail Operations, Inventory Audits, Merchandising, POS Systems, Team Coordination, Excel',
    4,
    'On-site',
    'Hobart',
    78000.00,
    92000.00,
    'Full-time',
    'Mid-level',
    (CURRENT_DATE + INTERVAL '33 days')::date,
    'Open'
FROM users u
WHERE u.email = 'greencart@example.com'
  AND NOT EXISTS
  (
      SELECT 1
      FROM job_postings jp
      WHERE jp.employer_id = u.id
        AND jp.job_title = 'Retail Operations Coordinator'
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
    'GreenCart Retail - Hobart',
    'Merchandise Planner',
    'Build seasonal assortment plans, monitor category sell-through, and support inventory allocation decisions for retail stores.',
    'Bachelor''s',
    'Merchandise Planning, Forecasting, Inventory Allocation, Excel, Category Analysis, Retail Reporting',
    3,
    'Hybrid',
    'Hobart',
    84000.00,
    98000.00,
    'Full-time',
    'Mid-level',
    (CURRENT_DATE + INTERVAL '37 days')::date,
    'Open'
FROM users u
WHERE u.email = 'greencart@example.com'
  AND NOT EXISTS
  (
      SELECT 1
      FROM job_postings jp
      WHERE jp.employer_id = u.id
        AND jp.job_title = 'Merchandise Planner'
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
    'StudioPulse Media - Melbourne',
    'Video Producer',
    'Lead pre-production planning, coordinate shoots, and deliver branded video projects for commercial clients.',
    'Bachelor''s',
    'Storyboarding, Shoot Scheduling, Adobe Premiere Pro, Client Briefing, Camera Direction, Production Planning',
    4,
    'Hybrid',
    'Melbourne',
    90000.00,
    108000.00,
    'Full-time',
    'Mid-level',
    (CURRENT_DATE + INTERVAL '29 days')::date,
    'Open'
FROM users u
WHERE u.email = 'studiopulse@example.com'
  AND NOT EXISTS
  (
      SELECT 1
      FROM job_postings jp
      WHERE jp.employer_id = u.id
        AND jp.job_title = 'Video Producer'
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
    'StudioPulse Media - Melbourne',
    'Audio Post-Production Editor',
    'Edit dialogue, clean tracks, balance mixes, and prepare final audio packages for digital and broadcast content.',
    'Bachelor''s',
    'Audio Mixing, Dialogue Editing, Adobe Audition, Sound Design, QC Review, Media Delivery',
    3,
    'On-site',
    'Melbourne',
    82000.00,
    96000.00,
    'Full-time',
    'Mid-level',
    (CURRENT_DATE + INTERVAL '31 days')::date,
    'Open'
FROM users u
WHERE u.email = 'studiopulse@example.com'
  AND NOT EXISTS
  (
      SELECT 1
      FROM job_postings jp
      WHERE jp.employer_id = u.id
        AND jp.job_title = 'Audio Post-Production Editor'
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
    'HarborLogix Services - Brisbane',
    'Fleet Compliance Officer',
    'Monitor driver records, audit transport documentation, coordinate corrective actions, and maintain fleet compliance registers.',
    'Bachelor''s',
    'Compliance Audits, Documentation Control, Incident Reporting, Regulatory Interpretation, Fleet Coordination, Excel',
    3,
    'On-site',
    'Brisbane',
    86000.00,
    101000.00,
    'Full-time',
    'Mid-level',
    (CURRENT_DATE + INTERVAL '35 days')::date,
    'Open'
FROM users u
WHERE u.email = 'harborlogix@example.com'
  AND NOT EXISTS
  (
      SELECT 1
      FROM job_postings jp
      WHERE jp.employer_id = u.id
        AND jp.job_title = 'Fleet Compliance Officer'
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
    'HarborLogix Services - Brisbane',
    'Warehouse Planning Supervisor',
    'Plan warehouse capacity, coordinate inbound scheduling, and optimize picking activity for high-volume logistics operations.',
    'Bachelor''s',
    'Warehouse Planning, Capacity Forecasting, Inventory Control, Shift Coordination, KPI Reporting, ERP',
    5,
    'On-site',
    'Brisbane',
    98000.00,
    118000.00,
    'Full-time',
    'Senior',
    (CURRENT_DATE + INTERVAL '41 days')::date,
    'Open'
FROM users u
WHERE u.email = 'harborlogix@example.com'
  AND NOT EXISTS
  (
      SELECT 1
      FROM job_postings jp
      WHERE jp.employer_id = u.id
        AND jp.job_title = 'Warehouse Planning Supervisor'
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
    'TerraField Environmental - Perth',
    'Environmental Monitoring Officer',
    'Coordinate field sampling schedules, review monitoring results, and prepare compliance summaries for environmental projects.',
    'Bachelor''s',
    'Environmental Monitoring, Sampling Plans, GIS, Data Logging, Field Reporting, Risk Assessments',
    3,
    'Hybrid',
    'Perth',
    88000.00,
    102000.00,
    'Full-time',
    'Mid-level',
    (CURRENT_DATE + INTERVAL '30 days')::date,
    'Open'
FROM users u
WHERE u.email = 'terrafield@example.com'
  AND NOT EXISTS
  (
      SELECT 1
      FROM job_postings jp
      WHERE jp.employer_id = u.id
        AND jp.job_title = 'Environmental Monitoring Officer'
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
    'TerraField Environmental - Perth',
    'Sustainability Reporting Coordinator',
    'Compile emissions, waste, and compliance data into client-ready sustainability reports and audit packs.',
    'Bachelor''s',
    'Regulatory Reporting, Sustainability Metrics, Stakeholder Coordination, Excel, Audit Preparation, Risk Assessments',
    2,
    'Remote',
    'Perth',
    79000.00,
    93000.00,
    'Full-time',
    'Entry-level',
    (CURRENT_DATE + INTERVAL '27 days')::date,
    'Open'
FROM users u
WHERE u.email = 'terrafield@example.com'
  AND NOT EXISTS
  (
      SELECT 1
      FROM job_postings jp
      WHERE jp.employer_id = u.id
        AND jp.job_title = 'Sustainability Reporting Coordinator'
  );

INSERT INTO users (full_name, email, password_hash, role, membership_status)
VALUES
    ('Omar Hassan', 'omar@example.com', 'Password123', 'candidate', 'premium'),
    ('Yasmin Cole', 'yasmin@example.com', 'Password123', 'candidate', 'free'),
    ('Mateo Alvarez', 'mateo@example.com', 'Password123', 'candidate', 'premium'),
    ('Hannah Brooks', 'hannah@example.com', 'Password123', 'candidate', 'free'),
    ('Isla McKenzie', 'isla@example.com', 'Password123', 'candidate', 'premium'),
    ('Karim Dabbagh', 'karim@example.com', 'Password123', 'candidate', 'free'),
    ('Skyline Build Group Pty Ltd', 'skylinebuild@example.com', 'Password123', 'employer', 'premium'),
    ('NorthRiver Foods Pty Ltd', 'northriver@example.com', 'Password123', 'employer', 'free'),
    ('CivicPath Services Pty Ltd', 'civicpath@example.com', 'Password123', 'employer', 'premium'),
    ('Aster Biolabs Pty Ltd', 'asterbiolabs@example.com', 'Password123', 'employer', 'free'),
    ('HarborView Hospitality Pty Ltd', 'harborview@example.com', 'Password123', 'employer', 'premium')
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
    'omar@example.com | 0400 777 111',
    'Bachelor''s',
    'Architecture',
    5,
    'BIM coordinator at FormGrid Projects; architectural draftsperson at Metro Design Studio.',
    'Revit, AutoCAD, BIM Coordination, Construction Documentation, Clash Detection, Bluebeam',
    'Hybrid',
    'Sydney',
    108000.00,
    'Full-time',
    'Built-environment professional focused on BIM delivery, documentation quality, and design coordination.',
    'https://portfolio.example.com/omar'
FROM users u
WHERE u.email = 'omar@example.com'
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
    'yasmin@example.com | 0400 777 222',
    'Master''s',
    'Public Health',
    4,
    'Community program officer at CityWell Outreach; health promotion coordinator at BetterLiving Network.',
    'Program Evaluation, Stakeholder Engagement, Community Outreach, Case Coordination, Reporting, Workshop Facilitation',
    'Hybrid',
    'Canberra',
    92000.00,
    'Full-time',
    'Public health professional with experience coordinating community initiatives, outcomes reporting, and service partnerships.',
    'https://portfolio.example.com/yasmin'
FROM users u
WHERE u.email = 'yasmin@example.com'
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
    'mateo@example.com | 0400 777 333',
    'Bachelor''s',
    'Food Science',
    6,
    'QA technologist at FreshPeak Foods; production quality analyst at GrainLine Manufacturing.',
    'HACCP, GMP, Root Cause Analysis, Quality Audits, CAPA, Production Documentation',
    'On-site',
    'Adelaide',
    98000.00,
    'Full-time',
    'Food manufacturing quality specialist focused on compliance, investigations, and continuous improvement on production lines.',
    'https://portfolio.example.com/mateo'
FROM users u
WHERE u.email = 'mateo@example.com'
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
    'hannah@example.com | 0400 777 444',
    'Bachelor''s',
    'Legal Studies',
    5,
    'Contracts officer at CivicCore Advisory; legal administrator at Southern Infrastructure Group.',
    'Contract Review, Policy Interpretation, Documentation Control, Procurement Support, Risk Registers, Stakeholder Liaison',
    'Hybrid',
    'Melbourne',
    99000.00,
    'Full-time',
    'Contracts and governance professional experienced in procurement documentation, compliance tracking, and stakeholder coordination.',
    'https://portfolio.example.com/hannah'
FROM users u
WHERE u.email = 'hannah@example.com'
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
    'isla@example.com | 0400 777 555',
    'Master''s',
    'Biomedical Science',
    3,
    'Research assistant at Genex Diagnostics; laboratory analyst at BioTrack Labs.',
    'PCR, ELISA, Laboratory Documentation, Sample Preparation, Data Analysis, Quality Control',
    'On-site',
    'Brisbane',
    91000.00,
    'Full-time',
    'Biomedical laboratory professional with experience in assay execution, documentation integrity, and regulated workflows.',
    'https://portfolio.example.com/isla'
FROM users u
WHERE u.email = 'isla@example.com'
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
    'karim@example.com | 0400 777 666',
    'Bachelor''s',
    'Hospitality Management',
    8,
    'Venue supervisor at Seabreeze Dining; duty manager at Lantern Wharf Hotel.',
    'Venue Operations, Staff Scheduling, Customer Service, POS Systems, Event Coordination, Inventory Ordering',
    'On-site',
    'Sydney',
    86000.00,
    'Full-time',
    'Hospitality operations leader experienced in venue coordination, service delivery, and shift management.',
    'https://portfolio.example.com/karim'
FROM users u
WHERE u.email = 'karim@example.com'
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
    'Skyline Build Group',
    'skylinebuild@example.com',
    '+61 2 9550 2001',
    'Manufacturing',
    '51-200',
    'Sydney',
    'Skyline Build Group delivers design coordination, construction documentation, and digital model workflows across commercial projects.',
    'https://skylinebuild.example.com'
FROM users u
WHERE u.email = 'skylinebuild@example.com'
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
    'NorthRiver Foods',
    'northriver@example.com',
    '+61 8 9550 2002',
    'Manufacturing',
    '201-500',
    'Adelaide',
    'NorthRiver Foods operates multi-line food manufacturing facilities with strong quality, safety, and continuous improvement programs.',
    'https://northriver.example.com'
FROM users u
WHERE u.email = 'northriver@example.com'
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
    'CivicPath Services',
    'civicpath@example.com',
    '+61 2 9550 2003',
    'Other',
    '51-200',
    'Canberra',
    'CivicPath delivers community service programs, grants administration, compliance reporting, and stakeholder support for public-sector clients.',
    'https://civicpath.example.com'
FROM users u
WHERE u.email = 'civicpath@example.com'
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
    'Aster Biolabs',
    'asterbiolabs@example.com',
    '+61 7 9550 2004',
    'Healthcare',
    '11-50',
    'Brisbane',
    'Aster Biolabs supports diagnostic assay development, sample processing, and regulated laboratory documentation for clinical partners.',
    'https://asterbiolabs.example.com'
FROM users u
WHERE u.email = 'asterbiolabs@example.com'
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
    'HarborView Hospitality',
    'harborview@example.com',
    '+61 2 9550 2005',
    'Other',
    '51-200',
    'Sydney',
    'HarborView Hospitality manages waterfront venues, events, and premium dining operations with a focus on service quality and shift coordination.',
    'https://harborview.example.com'
FROM users u
WHERE u.email = 'harborview@example.com'
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
    'Skyline Build Group - Sydney',
    'BIM Coordinator',
    'Coordinate consultant models, resolve clashes, and maintain digital design standards across active building projects.',
    'Bachelor''s',
    'Revit, BIM Coordination, Clash Detection, Construction Documentation, Navisworks, Bluebeam',
    4,
    'Hybrid',
    'Sydney',
    105000.00,
    128000.00,
    'Full-time',
    'Mid-level',
    (CURRENT_DATE + INTERVAL '39 days')::date,
    'Open'
FROM users u
WHERE u.email = 'skylinebuild@example.com'
  AND NOT EXISTS
  (
      SELECT 1
      FROM job_postings jp
      WHERE jp.employer_id = u.id
        AND jp.job_title = 'BIM Coordinator'
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
    'Skyline Build Group - Sydney',
    'Contracts Administrator',
    'Support procurement packages, maintain contract registers, and coordinate commercial documentation for project teams.',
    'Bachelor''s',
    'Contract Administration, Documentation Control, Procurement Support, Risk Registers, Stakeholder Coordination, Excel',
    3,
    'Hybrid',
    'Sydney',
    92000.00,
    112000.00,
    'Full-time',
    'Mid-level',
    (CURRENT_DATE + INTERVAL '32 days')::date,
    'Open'
FROM users u
WHERE u.email = 'skylinebuild@example.com'
  AND NOT EXISTS
  (
      SELECT 1
      FROM job_postings jp
      WHERE jp.employer_id = u.id
        AND jp.job_title = 'Contracts Administrator'
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
    'NorthRiver Foods - Adelaide',
    'Quality Assurance Technologist',
    'Investigate production deviations, maintain food safety documentation, and support quality systems across manufacturing lines.',
    'Bachelor''s',
    'HACCP, GMP, CAPA, Root Cause Analysis, Quality Audits, Production Documentation',
    4,
    'On-site',
    'Adelaide',
    93000.00,
    108000.00,
    'Full-time',
    'Mid-level',
    (CURRENT_DATE + INTERVAL '34 days')::date,
    'Open'
FROM users u
WHERE u.email = 'northriver@example.com'
  AND NOT EXISTS
  (
      SELECT 1
      FROM job_postings jp
      WHERE jp.employer_id = u.id
        AND jp.job_title = 'Quality Assurance Technologist'
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
    'NorthRiver Foods - Adelaide',
    'Production Planning Analyst',
    'Balance line schedules, review forecast inputs, and improve production planning decisions for packaged food operations.',
    'Bachelor''s',
    'Production Planning, Forecasting, Excel, ERP, Capacity Planning, KPI Reporting',
    3,
    'On-site',
    'Adelaide',
    87000.00,
    101000.00,
    'Full-time',
    'Mid-level',
    (CURRENT_DATE + INTERVAL '36 days')::date,
    'Open'
FROM users u
WHERE u.email = 'northriver@example.com'
  AND NOT EXISTS
  (
      SELECT 1
      FROM job_postings jp
      WHERE jp.employer_id = u.id
        AND jp.job_title = 'Production Planning Analyst'
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
    'CivicPath Services - Canberra',
    'Community Program Coordinator',
    'Coordinate delivery schedules, liaise with service partners, and track outcomes across community support programs.',
    'Bachelor''s',
    'Community Outreach, Program Coordination, Stakeholder Engagement, Reporting, Workshop Facilitation, Case Coordination',
    3,
    'Hybrid',
    'Canberra',
    84000.00,
    98000.00,
    'Full-time',
    'Mid-level',
    (CURRENT_DATE + INTERVAL '40 days')::date,
    'Open'
FROM users u
WHERE u.email = 'civicpath@example.com'
  AND NOT EXISTS
  (
      SELECT 1
      FROM job_postings jp
      WHERE jp.employer_id = u.id
        AND jp.job_title = 'Community Program Coordinator'
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
    'CivicPath Services - Canberra',
    'Grants Compliance Analyst',
    'Review funding documentation, prepare audit-ready files, and support compliance reporting for public sector programs.',
    'Bachelor''s',
    'Policy Interpretation, Compliance Reporting, Documentation Review, Risk Registers, Audit Preparation, Excel',
    2,
    'Remote',
    'Canberra',
    80000.00,
    94000.00,
    'Full-time',
    'Entry-level',
    (CURRENT_DATE + INTERVAL '31 days')::date,
    'Open'
FROM users u
WHERE u.email = 'civicpath@example.com'
  AND NOT EXISTS
  (
      SELECT 1
      FROM job_postings jp
      WHERE jp.employer_id = u.id
        AND jp.job_title = 'Grants Compliance Analyst'
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
    'Aster Biolabs - Brisbane',
    'Laboratory Quality Coordinator',
    'Maintain assay records, support deviation investigations, and coordinate quality documentation for diagnostic laboratory workflows.',
    'Bachelor''s',
    'Quality Control, Laboratory Documentation, CAPA, Sample Tracking, Audit Support, Data Review',
    3,
    'On-site',
    'Brisbane',
    89000.00,
    104000.00,
    'Full-time',
    'Mid-level',
    (CURRENT_DATE + INTERVAL '35 days')::date,
    'Open'
FROM users u
WHERE u.email = 'asterbiolabs@example.com'
  AND NOT EXISTS
  (
      SELECT 1
      FROM job_postings jp
      WHERE jp.employer_id = u.id
        AND jp.job_title = 'Laboratory Quality Coordinator'
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
    'Aster Biolabs - Brisbane',
    'Research Operations Assistant',
    'Prepare samples, support regulated lab workflows, and maintain data capture quality across biomedical projects.',
    'Bachelor''s',
    'Sample Preparation, Laboratory Support, Data Entry, PCR, ELISA, Documentation Control',
    1,
    'On-site',
    'Brisbane',
    72000.00,
    85000.00,
    'Full-time',
    'Entry-level',
    (CURRENT_DATE + INTERVAL '26 days')::date,
    'Open'
FROM users u
WHERE u.email = 'asterbiolabs@example.com'
  AND NOT EXISTS
  (
      SELECT 1
      FROM job_postings jp
      WHERE jp.employer_id = u.id
        AND jp.job_title = 'Research Operations Assistant'
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
    'HarborView Hospitality - Sydney',
    'Venue Supervisor',
    'Lead front-of-house operations, coordinate event shifts, and maintain service standards across a busy waterfront venue.',
    'Bachelor''s',
    'Venue Operations, Staff Scheduling, POS Systems, Customer Service, Event Coordination, Inventory Ordering',
    4,
    'On-site',
    'Sydney',
    78000.00,
    91000.00,
    'Full-time',
    'Mid-level',
    (CURRENT_DATE + INTERVAL '29 days')::date,
    'Open'
FROM users u
WHERE u.email = 'harborview@example.com'
  AND NOT EXISTS
  (
      SELECT 1
      FROM job_postings jp
      WHERE jp.employer_id = u.id
        AND jp.job_title = 'Venue Supervisor'
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
    'HarborView Hospitality - Sydney',
    'Events Operations Coordinator',
    'Coordinate run sheets, supplier logistics, and client-ready event operations for premium dining and function spaces.',
    'Bachelor''s',
    'Event Coordination, Supplier Liaison, Run Sheets, Venue Operations, Customer Service, Scheduling',
    2,
    'On-site',
    'Sydney',
    74000.00,
    86000.00,
    'Full-time',
    'Entry-level',
    (CURRENT_DATE + INTERVAL '24 days')::date,
    'Open'
FROM users u
WHERE u.email = 'harborview@example.com'
  AND NOT EXISTS
  (
      SELECT 1
      FROM job_postings jp
      WHERE jp.employer_id = u.id
        AND jp.job_title = 'Events Operations Coordinator'
  );

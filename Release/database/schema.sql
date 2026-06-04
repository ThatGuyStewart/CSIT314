-- ============================================================
-- Intelligent Talent Matching Platform - PostgreSQL Schema
-- CSIT314 Group Project
-- ============================================================

-- Database creation should be handled by the application.
-- This file should only contain schema objects for the current database.

CREATE EXTENSION IF NOT EXISTS pgcrypto;

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
    ('Platform Admin', 'admin@platform.com', crypt('Password123', gen_salt('bf')), 'admin', 'premium'),
    ('Alice Johnson', 'alice@example.com', crypt('Password123', gen_salt('bf')), 'candidate', 'free'),
    ('TechCorp Pty Ltd', 'techcorp@example.com', crypt('Password123', gen_salt('bf')), 'employer', 'free'),
    ('Brian Lee', 'brian@example.com', crypt('Password123', gen_salt('bf')), 'candidate', 'premium'),
    ('Chloe Martin', 'chloe@example.com', crypt('Password123', gen_salt('bf')), 'candidate', 'free'),
    ('Daniel Kim', 'daniel@example.com', crypt('Password123', gen_salt('bf')), 'candidate', 'free'),
    ('DataWorks Pty Ltd', 'dataworks@example.com', crypt('Password123', gen_salt('bf')), 'employer', 'premium'),
    ('HealthBridge Pty Ltd', 'healthbridge@example.com', crypt('Password123', gen_salt('bf')), 'employer', 'free'),
    ('FinEdge Pty Ltd', 'finedge@example.com', crypt('Password123', gen_salt('bf')), 'employer', 'premium'),
    ('EduSpark Pty Ltd', 'eduspark@example.com', crypt('Password123', gen_salt('bf')), 'employer', 'free'),
    ('BuildSmart Pty Ltd', 'buildsmart@example.com', crypt('Password123', gen_salt('bf')), 'employer', 'premium'),
    ('Mia Chen', 'mia@example.com', crypt('Password123', gen_salt('bf')), 'candidate', 'premium'),
    ('Noah Patel', 'noah@example.com', crypt('Password123', gen_salt('bf')), 'candidate', 'free'),
    ('Olivia Brown', 'olivia@example.com', crypt('Password123', gen_salt('bf')), 'candidate', 'premium'),
    ('Ethan Walker', 'ethan@example.com', crypt('Password123', gen_salt('bf')), 'candidate', 'free'),
    ('Sophia Nguyen', 'sophia@example.com', crypt('Password123', gen_salt('bf')), 'candidate', 'premium'),
    ('Liam Scott', 'liam@example.com', crypt('Password123', gen_salt('bf')), 'candidate', 'free'),
    ('Priya Raman', 'priya@example.com', crypt('Password123', gen_salt('bf')), 'candidate', 'free'),
    ('Lucas Meyer', 'lucas@example.com', crypt('Password123', gen_salt('bf')), 'candidate', 'premium'),
    ('Grace O''Connor', 'grace@example.com', crypt('Password123', gen_salt('bf')), 'candidate', 'free'),
    ('Elena Petrova', 'elena@example.com', crypt('Password123', gen_salt('bf')), 'candidate', 'premium'),
    ('GreenCart Retail Pty Ltd', 'greencart@example.com', crypt('Password123', gen_salt('bf')), 'employer', 'premium'),
    ('StudioPulse Media Pty Ltd', 'studiopulse@example.com', crypt('Password123', gen_salt('bf')), 'employer', 'free'),
    ('HarborLogix Services Pty Ltd', 'harborlogix@example.com', crypt('Password123', gen_salt('bf')), 'employer', 'premium'),
    ('TerraField Environmental Pty Ltd', 'terrafield@example.com', crypt('Password123', gen_salt('bf')), 'employer', 'free'),
    ('Omar Hassan', 'omar@example.com', crypt('Password123', gen_salt('bf')), 'candidate', 'premium'),
    ('Yasmin Cole', 'yasmin@example.com', crypt('Password123', gen_salt('bf')), 'candidate', 'free'),
    ('Mateo Alvarez', 'mateo@example.com', crypt('Password123', gen_salt('bf')), 'candidate', 'premium'),
    ('Hannah Brooks', 'hannah@example.com', crypt('Password123', gen_salt('bf')), 'candidate', 'free'),
    ('Isla McKenzie', 'isla@example.com', crypt('Password123', gen_salt('bf')), 'candidate', 'premium'),
    ('Karim Dabbagh', 'karim@example.com', crypt('Password123', gen_salt('bf')), 'candidate', 'free'),
    ('Skyline Build Group Pty Ltd', 'skylinebuild@example.com', crypt('Password123', gen_salt('bf')), 'employer', 'premium'),
    ('NorthRiver Foods Pty Ltd', 'northriver@example.com', crypt('Password123', gen_salt('bf')), 'employer', 'free'),
    ('CivicPath Services Pty Ltd', 'civicpath@example.com', crypt('Password123', gen_salt('bf')), 'employer', 'premium'),
    ('Aster Biolabs Pty Ltd', 'asterbiolabs@example.com', crypt('Password123', gen_salt('bf')), 'employer', 'free'),
    ('HarborView Hospitality Pty Ltd', 'harborview@example.com', crypt('Password123', gen_salt('bf')), 'employer', 'premium'),
    ('Amelia Reed', 'amelia@example.com', crypt('Password123', gen_salt('bf')), 'candidate', 'premium'),
    ('Jack Turner', 'jack@example.com', crypt('Password123', gen_salt('bf')), 'candidate', 'free'),
    ('Zoe Bennett', 'zoe@example.com', crypt('Password123', gen_salt('bf')), 'candidate', 'premium'),
    ('Caleb Foster', 'caleb@example.com', crypt('Password123', gen_salt('bf')), 'candidate', 'free'),
    ('Nina Shah', 'nina@example.com', crypt('Password123', gen_salt('bf')), 'candidate', 'premium'),
    ('Victor Lane', 'victor@example.com', crypt('Password123', gen_salt('bf')), 'candidate', 'free'),
    ('Ruby Collins', 'ruby@example.com', crypt('Password123', gen_salt('bf')), 'candidate', 'premium'),
    ('Aiden Murphy', 'aiden@example.com', crypt('Password123', gen_salt('bf')), 'candidate', 'free'),
    ('Stella Park', 'stella@example.com', crypt('Password123', gen_salt('bf')), 'candidate', 'premium'),
    ('Julian Cross', 'julian@example.com', crypt('Password123', gen_salt('bf')), 'candidate', 'free'),
    ('Layla Ibrahim', 'layla@example.com', crypt('Password123', gen_salt('bf')), 'candidate', 'premium'),
    ('Theo Grant', 'theo@example.com', crypt('Password123', gen_salt('bf')), 'candidate', 'free'),
    ('Maya Rossi', 'maya@example.com', crypt('Password123', gen_salt('bf')), 'candidate', 'premium'),
    ('Connor Blake', 'connor@example.com', crypt('Password123', gen_salt('bf')), 'candidate', 'free'),
    ('SolarNest Energy Pty Ltd', 'solarnest@example.com', crypt('Password123', gen_salt('bf')), 'employer', 'premium'),
    ('PeakLine Logistics Pty Ltd', 'peakline@example.com', crypt('Password123', gen_salt('bf')), 'employer', 'free'),
    ('BluePeak Finance Pty Ltd', 'bluepeakfinance@example.com', crypt('Password123', gen_salt('bf')), 'employer', 'premium'),
    ('Orchard Learning Pty Ltd', 'orchardlearning@example.com', crypt('Password123', gen_salt('bf')), 'employer', 'free'),
    ('NeonForge Studios Pty Ltd', 'neonforge@example.com', crypt('Password123', gen_salt('bf')), 'employer', 'premium'),
    ('SilverOak Health Pty Ltd', 'silveroakhealth@example.com', crypt('Password123', gen_salt('bf')), 'employer', 'free'),
    ('MetroGrid Systems Pty Ltd', 'metrogrid@example.com', crypt('Password123', gen_salt('bf')), 'employer', 'premium'),
    ('FreshHarbor Retail Pty Ltd', 'freshharbor@example.com', crypt('Password123', gen_salt('bf')), 'employer', 'free'),
    ('CoreAxis Manufacturing Pty Ltd', 'coreaxis@example.com', crypt('Password123', gen_salt('bf')), 'employer', 'premium'),
    ('BrightWave Civic Pty Ltd', 'brightwavecivic@example.com', crypt('Password123', gen_salt('bf')), 'employer', 'free'),
    ('TrueNorth Travel Pty Ltd', 'truenorthtravel@example.com', crypt('Password123', gen_salt('bf')), 'employer', 'premium'),
    ('Lumina Biotech Pty Ltd', 'luminabiotech@example.com', crypt('Password123', gen_salt('bf')), 'employer', 'free'),
    ('RedStone Property Pty Ltd', 'redstoneproperty@example.com', crypt('Password123', gen_salt('bf')), 'employer', 'premium'),
    ('PulsePoint Fitness Pty Ltd', 'pulsepointfitness@example.com', crypt('Password123', gen_salt('bf')), 'employer', 'free')
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
    v.contact_info,
    v.education,
    v.major,
    v.years_experience,
    v.work_experience,
    v.skills,
    v.preferred_work_mode,
    v.preferred_location,
    v.expected_salary,
    v.employment_type,
    v.summary,
    v.portfolio_url
FROM
(
    VALUES
    ('alice@example.com', 'alice@example.com | 0400 111 222', 'Bachelor''s', 'Computer Science', 3, 'Frontend developer at Startup Labs; internship at BlueWave Digital.', 'JavaScript, TypeScript, React, HTML, CSS, Node.js', 'Hybrid', 'Sydney', 85000.00, 'Full-time', 'Frontend-focused software developer with strong UI and JavaScript skills.', 'https://portfolio.example.com/alice'),
    ('brian@example.com', 'brian@example.com | 0400 222 333', 'Master''s', 'Software Engineering', 5, 'Backend engineer at FinStack; software engineer at CloudArc.', 'C++, C#, PostgreSQL, Docker, REST APIs, Azure', 'Remote', 'Melbourne', 115000.00, 'Full-time', 'Backend-focused engineer with cloud and database experience.', 'https://portfolio.example.com/brian'),
    ('chloe@example.com', 'chloe@example.com | 0400 333 444', 'Bachelor''s', 'Data Science', 2, 'Data analyst at Insight Labs; internship at City Analytics.', 'Python, SQL, Power BI, Pandas, Machine Learning, Excel', 'Hybrid', 'Brisbane', 82000.00, 'Full-time', 'Data-driven analyst with reporting and ML project experience.', 'https://portfolio.example.com/chloe'),
    ('daniel@example.com', 'daniel@example.com | 0400 444 555', 'Bachelor''s', 'Information Systems', 4, 'Full stack developer at RetailFlow; web developer at BrightApps.', 'JavaScript, TypeScript, React, Node.js, PostgreSQL, AWS', 'On-site', 'Sydney', 98000.00, 'Full-time', 'Full stack developer experienced in product delivery and cloud deployment.', 'https://portfolio.example.com/daniel'),
    ('mia@example.com', 'mia@example.com | 0400 555 101', 'Master''s', 'Finance and Analytics', 6, 'Senior financial analyst at CapitalNorth; BI analyst at LedgerPoint.', 'SQL, Power BI, Tableau, Financial Modelling, Excel, Risk Analysis', 'Hybrid', 'Sydney', 125000.00, 'Full-time', 'Finance analytics professional with strong reporting, forecasting, and stakeholder communication skills.', 'https://portfolio.example.com/mia'),
    ('noah@example.com', 'noah@example.com | 0400 555 202', 'Bachelor''s', 'Cybersecurity', 4, 'Security analyst at SecureGrid; SOC engineer at NetShield.', 'SIEM, Splunk, Incident Response, Network Security, Python, Vulnerability Management', 'Remote', 'Melbourne', 112000.00, 'Full-time', 'Cybersecurity specialist focused on monitoring, incident response, and secure operations.', 'https://portfolio.example.com/noah'),
    ('olivia@example.com', 'olivia@example.com | 0400 555 303', 'Bachelor''s', 'UX Design', 5, 'Product designer at LearnLoop; UX designer at Studio Pixel.', 'Figma, User Research, Prototyping, Design Systems, Accessibility, Wireframing', 'Hybrid', 'Adelaide', 98000.00, 'Full-time', 'UX designer with strengths in product discovery, accessibility, and collaborative design systems.', 'https://portfolio.example.com/olivia'),
    ('ethan@example.com', 'ethan@example.com | 0400 555 404', 'Bachelor''s', 'Mechanical Engineering', 7, 'Automation engineer at PlantCore; maintenance engineer at Allied Manufacturing.', 'PLC, SCADA, AutoCAD, Preventive Maintenance, IoT Sensors, Root Cause Analysis', 'On-site', 'Perth', 118000.00, 'Full-time', 'Industrial automation engineer experienced in plant reliability and manufacturing operations.', 'https://portfolio.example.com/ethan'),
    ('sophia@example.com', 'sophia@example.com | 0400 555 505', 'Master''s', 'Healthcare Informatics', 3, 'Clinical systems analyst at MedAxis; implementation consultant at CareFlow.', 'HL7, SQL, Healthcare Systems, Requirements Gathering, Data Mapping, UAT', 'Hybrid', 'Brisbane', 97000.00, 'Full-time', 'Healthcare technology analyst experienced in clinical workflows and system integrations.', 'https://portfolio.example.com/sophia'),
    ('liam@example.com', 'liam@example.com | 0400 555 606', 'Bachelor''s', 'Digital Marketing', 4, 'Growth marketer at BrightReach; content strategist at Wave Media.', 'SEO, Google Analytics, Content Strategy, Paid Search, Email Marketing, CRM', 'Remote', 'Gold Coast', 88000.00, 'Full-time', 'Digital marketer focused on acquisition, campaign analytics, and conversion optimization.', 'https://portfolio.example.com/liam'),
    ('priya@example.com', 'priya@example.com | 0400 666 111', 'Master''s', 'Supply Chain Management', 6, 'Supply planner at PortLink Logistics; inventory analyst at FreshRoute Distribution.', 'Demand Planning, Procurement, Inventory Control, ERP, Excel, Vendor Management', 'On-site', 'Brisbane', 104000.00, 'Full-time', 'Operations-focused supply chain professional with strong planning and vendor coordination experience.', 'https://portfolio.example.com/priya'),
    ('lucas@example.com', 'lucas@example.com | 0400 666 222', 'Bachelor''s', 'Film and Media Production', 5, 'Video editor at NorthFrame Studio; production coordinator at CityScreen Media.', 'Adobe Premiere Pro, After Effects, Storyboarding, Audio Mixing, Camera Operation, Script Breakdown', 'Hybrid', 'Melbourne', 93000.00, 'Full-time', 'Media production specialist focused on video delivery, post-production, and creative coordination.', 'https://portfolio.example.com/lucas'),
    ('grace@example.com', 'grace@example.com | 0400 666 333', 'Bachelor''s', 'Retail Management', 7, 'Store operations lead at MarketSquare; merchandising supervisor at ValueHome Retail.', 'Retail Operations, Merchandising, POS Systems, Inventory Audits, Team Leadership, Rostering', 'On-site', 'Hobart', 88000.00, 'Full-time', 'Retail operations leader experienced in merchandising execution, store standards, and frontline coaching.', 'https://portfolio.example.com/grace'),
    ('elena@example.com', 'elena@example.com | 0400 666 444', 'Master''s', 'Environmental Science', 4, 'Field compliance analyst at EcoTrack; environmental coordinator at Coastal Survey Group.', 'Environmental Auditing, GIS, Sampling Plans, Regulatory Reporting, Risk Assessments, Stakeholder Engagement', 'Hybrid', 'Perth', 97000.00, 'Full-time', 'Environmental compliance professional with experience in monitoring programs and sustainability reporting.', 'https://portfolio.example.com/elena'),
    ('omar@example.com', 'omar@example.com | 0400 777 111', 'Bachelor''s', 'Architecture', 5, 'BIM coordinator at FormGrid Projects; architectural draftsperson at Metro Design Studio.', 'Revit, AutoCAD, BIM Coordination, Construction Documentation, Clash Detection, Bluebeam', 'Hybrid', 'Sydney', 108000.00, 'Full-time', 'Built-environment professional focused on BIM delivery, documentation quality, and design coordination.', 'https://portfolio.example.com/omar'),
    ('yasmin@example.com', 'yasmin@example.com | 0400 777 222', 'Master''s', 'Public Health', 4, 'Community program officer at CityWell Outreach; health promotion coordinator at BetterLiving Network.', 'Program Evaluation, Stakeholder Engagement, Community Outreach, Case Coordination, Reporting, Workshop Facilitation', 'Hybrid', 'Canberra', 92000.00, 'Full-time', 'Public health professional with experience coordinating community initiatives, outcomes reporting, and service partnerships.', 'https://portfolio.example.com/yasmin'),
    ('mateo@example.com', 'mateo@example.com | 0400 777 333', 'Bachelor''s', 'Food Science', 6, 'QA technologist at FreshPeak Foods; production quality analyst at GrainLine Manufacturing.', 'HACCP, GMP, Root Cause Analysis, Quality Audits, CAPA, Production Documentation', 'On-site', 'Adelaide', 98000.00, 'Full-time', 'Food manufacturing quality specialist focused on compliance, investigations, and continuous improvement on production lines.', 'https://portfolio.example.com/mateo'),
    ('hannah@example.com', 'hannah@example.com | 0400 777 444', 'Bachelor''s', 'Legal Studies', 5, 'Contracts officer at CivicCore Advisory; legal administrator at Southern Infrastructure Group.', 'Contract Review, Policy Interpretation, Documentation Control, Procurement Support, Risk Registers, Stakeholder Liaison', 'Hybrid', 'Melbourne', 99000.00, 'Full-time', 'Contracts and governance professional experienced in procurement documentation, compliance tracking, and stakeholder coordination.', 'https://portfolio.example.com/hannah'),
    ('isla@example.com', 'isla@example.com | 0400 777 555', 'Master''s', 'Biomedical Science', 3, 'Research assistant at Genex Diagnostics; laboratory analyst at BioTrack Labs.', 'PCR, ELISA, Laboratory Documentation, Sample Preparation, Data Analysis, Quality Control', 'On-site', 'Brisbane', 91000.00, 'Full-time', 'Biomedical laboratory professional with experience in assay execution, documentation integrity, and regulated workflows.', 'https://portfolio.example.com/isla'),
    ('karim@example.com', 'karim@example.com | 0400 777 666', 'Bachelor''s', 'Hospitality Management', 8, 'Venue supervisor at Seabreeze Dining; duty manager at Lantern Wharf Hotel.', 'Venue Operations, Staff Scheduling, Customer Service, POS Systems, Event Coordination, Inventory Ordering', 'On-site', 'Sydney', 86000.00, 'Full-time', 'Hospitality operations leader experienced in venue coordination, service delivery, and shift management.', 'https://portfolio.example.com/karim'),
    ('amelia@example.com', 'amelia@example.com | 0400 888 101', 'Bachelor''s', 'Software Engineering', 4, 'Frontend engineer at Sunlit Apps; UI developer at Coastline Tech.', 'React, TypeScript, Design Systems, JavaScript, HTML, CSS', 'Hybrid', 'Sydney', 98000.00, 'Full-time', 'Frontend engineer focused on accessible interfaces and polished web products.', 'https://portfolio.example.com/amelia'),
    ('jack@example.com', 'jack@example.com | 0400 888 102', 'Bachelor''s', 'Data Engineering', 5, 'Data engineer at QueryPath; reporting analyst at Insight Forge.', 'Python, SQL, ETL, PostgreSQL, Power BI, Data Modelling', 'Hybrid', 'Melbourne', 112000.00, 'Full-time', 'Data platform specialist with strong reporting, warehousing, and automation experience.', 'https://portfolio.example.com/jack'),
    ('zoe@example.com', 'zoe@example.com | 0400 888 103', 'Bachelor''s', 'Interaction Design', 4, 'UX designer at BrightCourse; product designer at Pixel Atlas.', 'Figma, Prototyping, Accessibility, User Research, Design Systems, Journey Mapping', 'Hybrid', 'Brisbane', 96000.00, 'Full-time', 'UX professional experienced in research, prototyping, and user-centred product design.', 'https://portfolio.example.com/zoe'),
    ('caleb@example.com', 'caleb@example.com | 0400 888 104', 'Bachelor''s', 'Computer Science', 6, 'Cloud engineer at InfraWave; systems developer at DevHarbor.', 'Azure, Docker, CI/CD, Linux, C#, PostgreSQL', 'Remote', 'Sydney', 128000.00, 'Full-time', 'Cloud and backend engineer focused on resilient platforms and deployment automation.', 'https://portfolio.example.com/caleb'),
    ('nina@example.com', 'nina@example.com | 0400 888 105', 'Master''s', 'Finance', 5, 'Risk analyst at Capital Pulse; BI associate at Ledger Bridge.', 'Financial Modelling, SQL, Tableau, Risk Analysis, Excel, Forecasting', 'Hybrid', 'Sydney', 118000.00, 'Full-time', 'Finance analytics specialist with strengths in risk, reporting, and stakeholder communication.', 'https://portfolio.example.com/nina'),
    ('victor@example.com', 'victor@example.com | 0400 888 106', 'Bachelor''s', 'Logistics Management', 6, 'Warehouse planning lead at FastLane Distribution; logistics analyst at PortTrack.', 'Warehouse Planning, Inventory Control, ERP, Capacity Planning, KPI Reporting, Procurement', 'On-site', 'Brisbane', 101000.00, 'Full-time', 'Logistics professional experienced in warehouse planning and operational reporting.', 'https://portfolio.example.com/victor'),
    ('ruby@example.com', 'ruby@example.com | 0400 888 107', 'Bachelor''s', 'Marketing', 4, 'Campaign specialist at Growth Harbor; digital marketer at Retail Spark.', 'SEO, Google Analytics, Paid Search, CRM, Content Strategy, Email Marketing', 'Remote', 'Perth', 91000.00, 'Full-time', 'Digital marketer focused on acquisition, analytics, and campaign optimisation.', 'https://portfolio.example.com/ruby'),
    ('aiden@example.com', 'aiden@example.com | 0400 888 108', 'Bachelor''s', 'Mechanical Engineering', 7, 'Automation engineer at Motion Core; reliability specialist at PlantGrid.', 'PLC, SCADA, AutoCAD, Maintenance Planning, IoT Sensors, Troubleshooting', 'On-site', 'Perth', 121000.00, 'Full-time', 'Industrial automation engineer experienced in production systems and maintenance workflows.', 'https://portfolio.example.com/aiden'),
    ('stella@example.com', 'stella@example.com | 0400 888 109', 'Master''s', 'Health Informatics', 4, 'Clinical systems analyst at Medisphere; implementation specialist at CareBridge.', 'HL7, SQL, Healthcare Systems, UAT, Data Mapping, Requirements Gathering', 'Hybrid', 'Brisbane', 99000.00, 'Full-time', 'Health systems analyst skilled in integrations, workflow mapping, and stakeholder delivery.', 'https://portfolio.example.com/stella'),
    ('julian@example.com', 'julian@example.com | 0400 888 110', 'Bachelor''s', 'Media Production', 5, 'Video producer at Motion Foundry; editor at Northshore Media.', 'Adobe Premiere Pro, After Effects, Storyboarding, Audio Mixing, Client Briefing, Production Planning', 'Hybrid', 'Melbourne', 94000.00, 'Full-time', 'Media production professional with end-to-end video delivery experience.', 'https://portfolio.example.com/julian'),
    ('layla@example.com', 'layla@example.com | 0400 888 111', 'Master''s', 'Education Technology', 5, 'Learning designer at CourseSpring; product analyst at EduCloud.', 'Learning Design, User Research, Analytics, Stakeholder Management, Roadmapping, Content Strategy', 'Hybrid', 'Adelaide', 104000.00, 'Full-time', 'Edtech professional combining learning design, product insight, and stakeholder delivery.', 'https://portfolio.example.com/layla'),
    ('theo@example.com', 'theo@example.com | 0400 888 112', 'Bachelor''s', 'Retail Management', 6, 'Store operations lead at MarketHub; ecommerce coordinator at FreshMart.', 'Retail Operations, Merchandising, POS Systems, Category Analysis, Inventory Audits, Campaign Coordination', 'Hybrid', 'Hobart', 89000.00, 'Full-time', 'Retail operations and ecommerce coordinator with strong execution and reporting skills.', 'https://portfolio.example.com/theo'),
    ('maya@example.com', 'maya@example.com | 0400 888 113', 'Master''s', 'Biomedical Science', 3, 'Research assistant at BioCore Labs; quality analyst at Pathline Diagnostics.', 'PCR, ELISA, Laboratory Documentation, Quality Control, Sample Preparation, Data Review', 'On-site', 'Brisbane', 93000.00, 'Full-time', 'Biomedical laboratory professional experienced in regulated workflows and quality documentation.', 'https://portfolio.example.com/maya'),
    ('connor@example.com', 'connor@example.com | 0400 888 114', 'Bachelor''s', 'Public Administration', 5, 'Program officer at Civic Reach; grants analyst at Community Alliance.', 'Program Coordination, Compliance Reporting, Stakeholder Engagement, Audit Preparation, Case Coordination, Documentation Review', 'Hybrid', 'Canberra', 95000.00, 'Full-time', 'Community and public-sector operations professional focused on delivery, reporting, and compliance.', 'https://portfolio.example.com/connor')
) AS v(email, contact_info, education, major, years_experience, work_experience, skills, preferred_work_mode, preferred_location, expected_salary, employment_type, summary, portfolio_url)
INNER JOIN users u ON u.email = v.email
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
    v.company_name,
    v.company_email,
    v.company_phone,
    v.industry,
    v.company_size,
    v.company_location,
    v.company_description,
    v.website_url
FROM
(
    VALUES
    ('techcorp@example.com', 'TechCorp', 'techcorp@example.com', '+61 2 9000 1234', 'Technology', '51-200', 'Sydney', 'TechCorp builds modern hiring and workforce platforms for Australian businesses.', 'https://techcorp.example.com'),
    ('dataworks@example.com', 'DataWorks', 'dataworks@example.com', '+61 3 9000 5678', 'Technology', '11-50', 'Melbourne', 'DataWorks builds analytics tools and internal data platforms for scaling teams.', 'https://dataworks.example.com'),
    ('healthbridge@example.com', 'HealthBridge', 'healthbridge@example.com', '+61 7 9000 2468', 'Healthcare', '51-200', 'Brisbane', 'HealthBridge delivers digital healthcare products and patient-facing systems.', 'https://healthbridge.example.com'),
    ('finedge@example.com', 'FinEdge', 'finedge@example.com', '+61 2 9100 7788', 'Finance', '201-500', 'Sydney', 'FinEdge develops digital banking platforms, fraud analytics, and compliance tooling.', 'https://finedge.example.com'),
    ('eduspark@example.com', 'EduSpark', 'eduspark@example.com', '+61 3 9200 4455', 'Education', '11-50', 'Adelaide', 'EduSpark builds learning platforms, virtual classrooms, and curriculum analytics products.', 'https://eduspark.example.com'),
    ('buildsmart@example.com', 'BuildSmart', 'buildsmart@example.com', '+61 7 9300 1122', 'Manufacturing', '51-200', 'Perth', 'BuildSmart creates industrial automation, IoT monitoring, and maintenance planning systems.', 'https://buildsmart.example.com'),
    ('greencart@example.com', 'GreenCart Retail', 'greencart@example.com', '+61 3 9550 1001', 'Retail', '201-500', 'Hobart', 'GreenCart runs multi-site retail stores and fulfillment operations with a strong focus on inventory accuracy and store execution.', 'https://greencart.example.com'),
    ('studiopulse@example.com', 'StudioPulse Media', 'studiopulse@example.com', '+61 3 9550 1002', 'Media', '11-50', 'Melbourne', 'StudioPulse produces commercial, digital, and broadcast media content with in-house pre-production and post-production teams.', 'https://studiopulse.example.com'),
    ('harborlogix@example.com', 'HarborLogix Services', 'harborlogix@example.com', '+61 7 9550 1003', 'Other', '51-200', 'Brisbane', 'HarborLogix manages fleet coordination, warehouse planning, and transport compliance services for industrial clients.', 'https://harborlogix.example.com'),
    ('terrafield@example.com', 'TerraField Environmental', 'terrafield@example.com', '+61 8 9550 1004', 'Other', '11-50', 'Perth', 'TerraField delivers monitoring, compliance, and sustainability reporting services for land, water, and industrial projects.', 'https://terrafield.example.com'),
    ('skylinebuild@example.com', 'Skyline Build Group', 'skylinebuild@example.com', '+61 2 9550 2001', 'Manufacturing', '51-200', 'Sydney', 'Skyline Build Group delivers design coordination, construction documentation, and digital model workflows across commercial projects.', 'https://skylinebuild.example.com'),
    ('northriver@example.com', 'NorthRiver Foods', 'northriver@example.com', '+61 8 9550 2002', 'Manufacturing', '201-500', 'Adelaide', 'NorthRiver Foods operates multi-line food manufacturing facilities with strong quality, safety, and continuous improvement programs.', 'https://northriver.example.com'),
    ('civicpath@example.com', 'CivicPath Services', 'civicpath@example.com', '+61 2 9550 2003', 'Other', '51-200', 'Canberra', 'CivicPath delivers community service programs, grants administration, compliance reporting, and stakeholder support for public-sector clients.', 'https://civicpath.example.com'),
    ('asterbiolabs@example.com', 'Aster Biolabs', 'asterbiolabs@example.com', '+61 7 9550 2004', 'Healthcare', '11-50', 'Brisbane', 'Aster Biolabs supports diagnostic assay development, sample processing, and regulated laboratory documentation for clinical partners.', 'https://asterbiolabs.example.com'),
    ('harborview@example.com', 'HarborView Hospitality', 'harborview@example.com', '+61 2 9550 2005', 'Other', '51-200', 'Sydney', 'HarborView Hospitality manages waterfront venues, events, and premium dining operations with a focus on service quality and shift coordination.', 'https://harborview.example.com'),
    ('solarnest@example.com', 'SolarNest Energy', 'solarnest@example.com', '+61 2 9660 3001', 'Other', '51-200', 'Sydney', 'SolarNest builds customer-facing clean energy platforms, reporting dashboards, and grid integration workflows.', 'https://solarnest.example.com'),
    ('peakline@example.com', 'PeakLine Logistics', 'peakline@example.com', '+61 7 9660 3002', 'Other', '201-500', 'Brisbane', 'PeakLine coordinates warehouse operations, freight planning, and logistics performance reporting for national clients.', 'https://peakline.example.com'),
    ('bluepeakfinance@example.com', 'BluePeak Finance', 'bluepeakfinance@example.com', '+61 2 9660 3003', 'Finance', '51-200', 'Sydney', 'BluePeak Finance delivers digital lending, reporting, and risk analytics products for business customers.', 'https://bluepeakfinance.example.com'),
    ('orchardlearning@example.com', 'Orchard Learning', 'orchardlearning@example.com', '+61 8 9660 3004', 'Education', '11-50', 'Adelaide', 'Orchard Learning creates digital classroom experiences and curriculum tools for schools and training providers.', 'https://orchardlearning.example.com'),
    ('neonforge@example.com', 'NeonForge Studios', 'neonforge@example.com', '+61 3 9660 3005', 'Media', '11-50', 'Melbourne', 'NeonForge Studios produces digital campaigns, branded content, and motion graphics for commercial partners.', 'https://neonforge.example.com'),
    ('silveroakhealth@example.com', 'SilverOak Health', 'silveroakhealth@example.com', '+61 7 9660 3006', 'Healthcare', '51-200', 'Brisbane', 'SilverOak Health supports digital health delivery, clinical systems, and patient data operations across care programs.', 'https://silveroakhealth.example.com'),
    ('metrogrid@example.com', 'MetroGrid Systems', 'metrogrid@example.com', '+61 2 9660 3007', 'Technology', '51-200', 'Sydney', 'MetroGrid Systems builds backend platforms, infrastructure tooling, and internal engineering services for enterprise clients.', 'https://metrogrid.example.com'),
    ('freshharbor@example.com', 'FreshHarbor Retail', 'freshharbor@example.com', '+61 3 9660 3008', 'Retail', '201-500', 'Hobart', 'FreshHarbor runs omnichannel retail operations with strong focus on merchandising insight and digital customer growth.', 'https://freshharbor.example.com'),
    ('coreaxis@example.com', 'CoreAxis Manufacturing', 'coreaxis@example.com', '+61 8 9660 3009', 'Manufacturing', '201-500', 'Perth', 'CoreAxis Manufacturing operates high-throughput industrial sites with strong automation, planning, and maintenance systems.', 'https://coreaxis.example.com'),
    ('brightwavecivic@example.com', 'BrightWave Civic', 'brightwavecivic@example.com', '+61 2 9660 3010', 'Other', '51-200', 'Canberra', 'BrightWave Civic delivers grants administration, community services coordination, and compliance reporting for public programs.', 'https://brightwavecivic.example.com'),
    ('truenorthtravel@example.com', 'TrueNorth Travel', 'truenorthtravel@example.com', '+61 3 9660 3011', 'Other', '51-200', 'Melbourne', 'TrueNorth Travel designs customer journeys, digital campaigns, and operational support systems for premium travel brands.', 'https://truenorthtravel.example.com'),
    ('luminabiotech@example.com', 'Lumina Biotech', 'luminabiotech@example.com', '+61 7 9660 3012', 'Healthcare', '11-50', 'Brisbane', 'Lumina Biotech supports regulated biomedical workflows, research operations, and diagnostic quality systems.', 'https://luminabiotech.example.com'),
    ('redstoneproperty@example.com', 'RedStone Property', 'redstoneproperty@example.com', '+61 2 9660 3013', 'Other', '51-200', 'Sydney', 'RedStone Property manages digital project delivery, property operations, and document coordination across commercial assets.', 'https://redstoneproperty.example.com'),
    ('pulsepointfitness@example.com', 'PulsePoint Fitness', 'pulsepointfitness@example.com', '+61 8 9660 3014', 'Other', '11-50', 'Perth', 'PulsePoint Fitness operates wellness venues and growth campaigns with strong focus on member experience and retention.', 'https://pulsepointfitness.example.com')
) AS v(email, company_name, company_email, company_phone, industry, company_size, company_location, company_description, website_url)
INNER JOIN users u ON u.email = v.email
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
    v.company_info,
    v.job_title,
    v.job_description,
    v.required_education,
    v.required_skills,
    v.required_experience,
    v.work_mode,
    v.job_location,
    v.salary_min,
    v.salary_max,
    v.job_type,
    v.career_level,
    (CURRENT_DATE + (v.deadline_days || ' days')::interval)::date,
    v.status
FROM
(
    VALUES
    ('techcorp@example.com', 'TechCorp - Sydney', 'Frontend Developer', 'Build responsive UI features for the TalentMatch platform.', 'Bachelor''s', 'JavaScript, TypeScript, React, HTML, CSS', 2, 'Hybrid', 'Sydney', 80000.00, 100000.00, 'Full-time', 'Mid-level', 30, 'Open'),
    ('techcorp@example.com', 'TechCorp - Sydney', 'Backend Developer', 'Design APIs and PostgreSQL-backed services for the TalentMatch platform.', 'Bachelor''s', 'C++, PostgreSQL, REST APIs, SQL', 3, 'On-site', 'Sydney', 90000.00, 115000.00, 'Full-time', 'Mid-level', 45, 'Open'),
    ('dataworks@example.com', 'DataWorks - Melbourne', 'Data Analyst', 'Build dashboards, analyze datasets, and support reporting for internal and client-facing analytics products.', 'Bachelor''s', 'Python, SQL, Power BI, Excel, Pandas', 2, 'Hybrid', 'Melbourne', 85000.00, 105000.00, 'Full-time', 'Mid-level', 35, 'Open'),
    ('healthbridge@example.com', 'HealthBridge - Brisbane', 'Full Stack Developer', 'Develop patient-facing web features and backend integrations for digital healthcare platforms.', 'Bachelor''s', 'JavaScript, TypeScript, React, Node.js, PostgreSQL', 3, 'On-site', 'Brisbane', 95000.00, 120000.00, 'Full-time', 'Mid-level', 40, 'Open'),
    ('dataworks@example.com', 'DataWorks - Melbourne', 'Cloud Engineer', 'Maintain cloud infrastructure, deployment pipelines, and platform reliability for analytics services.', 'Bachelor''s', 'AWS, Docker, CI/CD, PostgreSQL, Linux', 4, 'Remote', 'Melbourne', 110000.00, 135000.00, 'Full-time', 'Senior', 50, 'Open'),
    ('finedge@example.com', 'FinEdge - Sydney', 'Fraud Analytics Specialist', 'Build fraud monitoring dashboards, investigate anomalies, and improve transaction risk models.', 'Bachelor''s', 'SQL, Python, Risk Analysis, Tableau, Fraud Detection, Statistics', 4, 'Hybrid', 'Sydney', 105000.00, 128000.00, 'Full-time', 'Mid-level', 32, 'Open'),
    ('finedge@example.com', 'FinEdge - Sydney', 'Cybersecurity Analyst', 'Monitor threats, review security events, and strengthen controls across banking platforms.', 'Bachelor''s', 'SIEM, Splunk, Incident Response, Python, Network Security, IAM', 3, 'Remote', 'Sydney', 110000.00, 132000.00, 'Full-time', 'Mid-level', 38, 'Open'),
    ('eduspark@example.com', 'EduSpark - Adelaide', 'UX Designer', 'Design intuitive student and teacher workflows for a modern learning platform.', 'Bachelor''s', 'Figma, User Research, Prototyping, Accessibility, Design Systems, Wireframing', 3, 'Hybrid', 'Adelaide', 90000.00, 108000.00, 'Full-time', 'Mid-level', 28, 'Open'),
    ('eduspark@example.com', 'EduSpark - Adelaide', 'Learning Platform Product Manager', 'Coordinate roadmap delivery, gather education-sector requirements, and align cross-functional teams.', 'Bachelor''s', 'Product Management, Stakeholder Management, Agile, User Stories, Analytics, Roadmapping', 5, 'Hybrid', 'Adelaide', 115000.00, 140000.00, 'Full-time', 'Senior', 44, 'Open'),
    ('buildsmart@example.com', 'BuildSmart - Perth', 'Industrial Automation Engineer', 'Design and maintain PLC-driven automation workflows for factory monitoring and throughput optimization.', 'Bachelor''s', 'PLC, SCADA, AutoCAD, IoT Sensors, Preventive Maintenance, Troubleshooting', 5, 'On-site', 'Perth', 118000.00, 145000.00, 'Full-time', 'Senior', 36, 'Open'),
    ('healthbridge@example.com', 'HealthBridge - Brisbane', 'Healthcare Systems Analyst', 'Support healthcare product implementations, data mapping, and workflow improvements with clinical teams.', 'Bachelor''s', 'HL7, SQL, Requirements Gathering, Data Mapping, UAT, Healthcare Systems', 2, 'Hybrid', 'Brisbane', 92000.00, 108000.00, 'Full-time', 'Entry-level', 34, 'Open'),
    ('buildsmart@example.com', 'BuildSmart - Perth', 'Digital Marketing Specialist', 'Lead industrial product campaigns, optimize digital acquisition, and improve lead generation reporting.', 'Bachelor''s', 'SEO, Google Analytics, Paid Search, Content Strategy, CRM, Email Marketing', 3, 'Remote', 'Perth', 82000.00, 98000.00, 'Full-time', 'Mid-level', 26, 'Open'),
    ('greencart@example.com', 'GreenCart Retail - Hobart', 'Retail Operations Coordinator', 'Coordinate stock transfers, store execution plans, roster inputs, and daily retail operations reporting across multiple sites.', 'Bachelor''s', 'Retail Operations, Inventory Audits, Merchandising, POS Systems, Team Coordination, Excel', 4, 'On-site', 'Hobart', 78000.00, 92000.00, 'Full-time', 'Mid-level', 33, 'Open'),
    ('greencart@example.com', 'GreenCart Retail - Hobart', 'Merchandise Planner', 'Build seasonal assortment plans, monitor category sell-through, and support inventory allocation decisions for retail stores.', 'Bachelor''s', 'Merchandise Planning, Forecasting, Inventory Allocation, Excel, Category Analysis, Retail Reporting', 3, 'Hybrid', 'Hobart', 84000.00, 98000.00, 'Full-time', 'Mid-level', 37, 'Open'),
    ('studiopulse@example.com', 'StudioPulse Media - Melbourne', 'Video Producer', 'Lead pre-production planning, coordinate shoots, and deliver branded video projects for commercial clients.', 'Bachelor''s', 'Storyboarding, Shoot Scheduling, Adobe Premiere Pro, Client Briefing, Camera Direction, Production Planning', 4, 'Hybrid', 'Melbourne', 90000.00, 108000.00, 'Full-time', 'Mid-level', 29, 'Open'),
    ('studiopulse@example.com', 'StudioPulse Media - Melbourne', 'Audio Post-Production Editor', 'Edit dialogue, clean tracks, balance mixes, and prepare final audio packages for digital and broadcast content.', 'Bachelor''s', 'Audio Mixing, Dialogue Editing, Adobe Audition, Sound Design, QC Review, Media Delivery', 3, 'On-site', 'Melbourne', 82000.00, 96000.00, 'Full-time', 'Mid-level', 31, 'Open'),
    ('harborlogix@example.com', 'HarborLogix Services - Brisbane', 'Fleet Compliance Officer', 'Monitor driver records, audit transport documentation, coordinate corrective actions, and maintain fleet compliance registers.', 'Bachelor''s', 'Compliance Audits, Documentation Control, Incident Reporting, Regulatory Interpretation, Fleet Coordination, Excel', 3, 'On-site', 'Brisbane', 86000.00, 101000.00, 'Full-time', 'Mid-level', 35, 'Open'),
    ('harborlogix@example.com', 'HarborLogix Services - Brisbane', 'Warehouse Planning Supervisor', 'Plan warehouse capacity, coordinate inbound scheduling, and optimize picking activity for high-volume logistics operations.', 'Bachelor''s', 'Warehouse Planning, Capacity Forecasting, Inventory Control, Shift Coordination, KPI Reporting, ERP', 5, 'On-site', 'Brisbane', 98000.00, 118000.00, 'Full-time', 'Senior', 41, 'Open'),
    ('terrafield@example.com', 'TerraField Environmental - Perth', 'Environmental Monitoring Officer', 'Coordinate field sampling schedules, review monitoring results, and prepare compliance summaries for environmental projects.', 'Bachelor''s', 'Environmental Monitoring, Sampling Plans, GIS, Data Logging, Field Reporting, Risk Assessments', 3, 'Hybrid', 'Perth', 88000.00, 102000.00, 'Full-time', 'Mid-level', 30, 'Open'),
    ('terrafield@example.com', 'TerraField Environmental - Perth', 'Sustainability Reporting Coordinator', 'Compile emissions, waste, and compliance data into client-ready sustainability reports and audit packs.', 'Bachelor''s', 'Regulatory Reporting, Sustainability Metrics, Stakeholder Coordination, Excel, Audit Preparation, Risk Assessments', 2, 'Remote', 'Perth', 79000.00, 93000.00, 'Full-time', 'Entry-level', 27, 'Open'),
    ('skylinebuild@example.com', 'Skyline Build Group - Sydney', 'BIM Coordinator', 'Coordinate consultant models, resolve clashes, and maintain digital design standards across active building projects.', 'Bachelor''s', 'Revit, BIM Coordination, Clash Detection, Construction Documentation, Navisworks, Bluebeam', 4, 'Hybrid', 'Sydney', 105000.00, 128000.00, 'Full-time', 'Mid-level', 39, 'Open'),
    ('skylinebuild@example.com', 'Skyline Build Group - Sydney', 'Contracts Administrator', 'Support procurement packages, maintain contract registers, and coordinate commercial documentation for project teams.', 'Bachelor''s', 'Contract Administration, Documentation Control, Procurement Support, Risk Registers, Stakeholder Coordination, Excel', 3, 'Hybrid', 'Sydney', 92000.00, 112000.00, 'Full-time', 'Mid-level', 32, 'Open'),
    ('northriver@example.com', 'NorthRiver Foods - Adelaide', 'Quality Assurance Technologist', 'Investigate production deviations, maintain food safety documentation, and support quality systems across manufacturing lines.', 'Bachelor''s', 'HACCP, GMP, CAPA, Root Cause Analysis, Quality Audits, Production Documentation', 4, 'On-site', 'Adelaide', 93000.00, 108000.00, 'Full-time', 'Mid-level', 34, 'Open'),
    ('northriver@example.com', 'NorthRiver Foods - Adelaide', 'Production Planning Analyst', 'Balance line schedules, review forecast inputs, and improve production planning decisions for packaged food operations.', 'Bachelor''s', 'Production Planning, Forecasting, Excel, ERP, Capacity Planning, KPI Reporting', 3, 'On-site', 'Adelaide', 87000.00, 101000.00, 'Full-time', 'Mid-level', 36, 'Open'),
    ('civicpath@example.com', 'CivicPath Services - Canberra', 'Community Program Coordinator', 'Coordinate delivery schedules, liaise with service partners, and track outcomes across community support programs.', 'Bachelor''s', 'Community Outreach, Program Coordination, Stakeholder Engagement, Reporting, Workshop Facilitation, Case Coordination', 3, 'Hybrid', 'Canberra', 84000.00, 98000.00, 'Full-time', 'Mid-level', 40, 'Open'),
    ('civicpath@example.com', 'CivicPath Services - Canberra', 'Grants Compliance Analyst', 'Review funding documentation, prepare audit-ready files, and support compliance reporting for public sector programs.', 'Bachelor''s', 'Policy Interpretation, Compliance Reporting, Documentation Review, Risk Registers, Audit Preparation, Excel', 2, 'Remote', 'Canberra', 80000.00, 94000.00, 'Full-time', 'Entry-level', 31, 'Open'),
    ('asterbiolabs@example.com', 'Aster Biolabs - Brisbane', 'Laboratory Quality Coordinator', 'Maintain assay records, support deviation investigations, and coordinate quality documentation for diagnostic laboratory workflows.', 'Bachelor''s', 'Quality Control, Laboratory Documentation, CAPA, Sample Tracking, Audit Support, Data Review', 3, 'On-site', 'Brisbane', 89000.00, 104000.00, 'Full-time', 'Mid-level', 35, 'Open'),
    ('asterbiolabs@example.com', 'Aster Biolabs - Brisbane', 'Research Operations Assistant', 'Prepare samples, support regulated lab workflows, and maintain data capture quality across biomedical projects.', 'Bachelor''s', 'Sample Preparation, Laboratory Support, Data Entry, PCR, ELISA, Documentation Control', 1, 'On-site', 'Brisbane', 72000.00, 85000.00, 'Full-time', 'Entry-level', 26, 'Open'),
    ('harborview@example.com', 'HarborView Hospitality - Sydney', 'Venue Supervisor', 'Lead front-of-house operations, coordinate event shifts, and maintain service standards across a busy waterfront venue.', 'Bachelor''s', 'Venue Operations, Staff Scheduling, POS Systems, Customer Service, Event Coordination, Inventory Ordering', 4, 'On-site', 'Sydney', 78000.00, 91000.00, 'Full-time', 'Mid-level', 29, 'Open'),
    ('harborview@example.com', 'HarborView Hospitality - Sydney', 'Events Operations Coordinator', 'Coordinate run sheets, supplier logistics, and client-ready event operations for premium dining and function spaces.', 'Bachelor''s', 'Event Coordination, Supplier Liaison, Run Sheets, Venue Operations, Customer Service, Scheduling', 2, 'On-site', 'Sydney', 74000.00, 86000.00, 'Full-time', 'Entry-level', 24, 'Open'),
    ('solarnest@example.com', 'SolarNest Energy - Sydney', 'Frontend Platform Engineer', 'Build customer-facing energy dashboards and modern web interfaces for clean-tech products.', 'Bachelor''s', 'React, TypeScript, JavaScript, HTML, CSS, Design Systems', 3, 'Hybrid', 'Sydney', 95000.00, 118000.00, 'Full-time', 'Mid-level', 34, 'Open'),
    ('solarnest@example.com', 'SolarNest Energy - Sydney', 'Cloud Integration Engineer', 'Support cloud services, backend integrations, and deployment reliability for energy reporting systems.', 'Bachelor''s', 'Azure, Docker, CI/CD, PostgreSQL, C#, Linux', 4, 'Remote', 'Sydney', 118000.00, 138000.00, 'Full-time', 'Senior', 41, 'Open'),
    ('peakline@example.com', 'PeakLine Logistics - Brisbane', 'Logistics Operations Analyst', 'Analyse freight and warehouse performance data to improve delivery planning and service outcomes.', 'Bachelor''s', 'SQL, Excel, KPI Reporting, Inventory Control, Procurement, Logistics', 3, 'On-site', 'Brisbane', 86000.00, 101000.00, 'Full-time', 'Mid-level', 32, 'Open'),
    ('peakline@example.com', 'PeakLine Logistics - Brisbane', 'Warehouse Systems Coordinator', 'Coordinate warehouse workflows, system updates, and operational reporting across distribution sites.', 'Bachelor''s', 'Warehouse Planning, ERP, Inventory Control, Capacity Planning, Excel, Shift Coordination', 4, 'On-site', 'Brisbane', 92000.00, 108000.00, 'Full-time', 'Mid-level', 38, 'Open'),
    ('bluepeakfinance@example.com', 'BluePeak Finance - Sydney', 'Financial Risk Analyst', 'Model portfolio risk, investigate anomalies, and support data-driven lending decisions.', 'Bachelor''s', 'Financial Modelling, SQL, Risk Analysis, Excel, Forecasting, Tableau', 4, 'Hybrid', 'Sydney', 108000.00, 129000.00, 'Full-time', 'Mid-level', 35, 'Open'),
    ('bluepeakfinance@example.com', 'BluePeak Finance - Sydney', 'Data Reporting Engineer', 'Build reporting pipelines and finance dashboards for internal analytics and compliance teams.', 'Bachelor''s', 'Python, SQL, ETL, PostgreSQL, Power BI, Data Modelling', 4, 'Hybrid', 'Sydney', 112000.00, 132000.00, 'Full-time', 'Senior', 43, 'Open'),
    ('orchardlearning@example.com', 'Orchard Learning - Adelaide', 'Learning Experience Designer', 'Design digital learning journeys, prototypes, and student-centred experiences for classroom products.', 'Bachelor''s', 'Figma, User Research, Prototyping, Accessibility, Learning Design, Design Systems', 3, 'Hybrid', 'Adelaide', 90000.00, 106000.00, 'Full-time', 'Mid-level', 29, 'Open'),
    ('orchardlearning@example.com', 'Orchard Learning - Adelaide', 'Education Product Analyst', 'Support roadmap decisions with user insights, analytics, and curriculum-aligned product recommendations.', 'Bachelor''s', 'Analytics, Stakeholder Management, Roadmapping, User Research, Reporting, Content Strategy', 4, 'Hybrid', 'Adelaide', 98000.00, 116000.00, 'Full-time', 'Mid-level', 37, 'Open'),
    ('neonforge@example.com', 'NeonForge Studios - Melbourne', 'Video Content Producer', 'Lead pre-production, coordinate shoots, and deliver high-quality branded video content.', 'Bachelor''s', 'Adobe Premiere Pro, Storyboarding, Client Briefing, Production Planning, Camera Direction, Scheduling', 4, 'Hybrid', 'Melbourne', 91000.00, 108000.00, 'Full-time', 'Mid-level', 30, 'Open'),
    ('neonforge@example.com', 'NeonForge Studios - Melbourne', 'Motion Graphics Designer', 'Create polished animations and edit media packages for campaign and social video delivery.', 'Bachelor''s', 'After Effects, Adobe Premiere Pro, Motion Graphics, Storyboarding, Design Systems, Audio Mixing', 3, 'Hybrid', 'Melbourne', 88000.00, 102000.00, 'Full-time', 'Mid-level', 33, 'Open'),
    ('silveroakhealth@example.com', 'SilverOak Health - Brisbane', 'Clinical Systems Analyst', 'Map workflows, coordinate testing, and support digital health implementation outcomes.', 'Bachelor''s', 'HL7, SQL, Requirements Gathering, UAT, Data Mapping, Healthcare Systems', 3, 'Hybrid', 'Brisbane', 92000.00, 108000.00, 'Full-time', 'Mid-level', 34, 'Open'),
    ('silveroakhealth@example.com', 'SilverOak Health - Brisbane', 'Health Data Integration Specialist', 'Support clinical data movement, integration validation, and platform configuration across care systems.', 'Bachelor''s', 'HL7, SQL, Data Mapping, Integration Testing, Requirements Gathering, Healthcare Systems', 4, 'Hybrid', 'Brisbane', 98000.00, 116000.00, 'Full-time', 'Mid-level', 40, 'Open'),
    ('metrogrid@example.com', 'MetroGrid Systems - Sydney', 'Backend Platform Engineer', 'Build backend services, APIs, and database-backed platform capabilities for enterprise systems.', 'Bachelor''s', 'C#, PostgreSQL, REST APIs, SQL, Docker, Azure', 4, 'Hybrid', 'Sydney', 110000.00, 132000.00, 'Full-time', 'Senior', 39, 'Open'),
    ('metrogrid@example.com', 'MetroGrid Systems - Sydney', 'DevOps Reliability Engineer', 'Maintain deployment pipelines, infrastructure automation, and service reliability across shared platforms.', 'Bachelor''s', 'Azure, Docker, CI/CD, Linux, PostgreSQL, Monitoring', 5, 'Remote', 'Sydney', 118000.00, 140000.00, 'Full-time', 'Senior', 44, 'Open'),
    ('freshharbor@example.com', 'FreshHarbor Retail - Hobart', 'Retail Insights Analyst', 'Build category and customer reporting to improve retail decisions across stores and digital channels.', 'Bachelor''s', 'SQL, Excel, Category Analysis, Merchandising, Power BI, Retail Reporting', 3, 'Hybrid', 'Hobart', 84000.00, 98000.00, 'Full-time', 'Mid-level', 31, 'Open'),
    ('freshharbor@example.com', 'FreshHarbor Retail - Hobart', 'Ecommerce Campaign Specialist', 'Plan campaigns, analyse acquisition performance, and optimise digital retail customer journeys.', 'Bachelor''s', 'SEO, Google Analytics, Paid Search, CRM, Content Strategy, Email Marketing', 3, 'Hybrid', 'Hobart', 82000.00, 95000.00, 'Full-time', 'Mid-level', 28, 'Open'),
    ('coreaxis@example.com', 'CoreAxis Manufacturing - Perth', 'Industrial Automation Engineer', 'Design and maintain automation workflows for manufacturing throughput, safety, and reliability.', 'Bachelor''s', 'PLC, SCADA, AutoCAD, IoT Sensors, Troubleshooting, Preventive Maintenance', 5, 'On-site', 'Perth', 118000.00, 145000.00, 'Full-time', 'Senior', 36, 'Open'),
    ('coreaxis@example.com', 'CoreAxis Manufacturing - Perth', 'Maintenance Systems Planner', 'Coordinate maintenance schedules, system registers, and plant improvement reporting for industrial assets.', 'Bachelor''s', 'Maintenance Planning, KPI Reporting, ERP, Root Cause Analysis, Inventory Control, Scheduling', 4, 'On-site', 'Perth', 98000.00, 118000.00, 'Full-time', 'Mid-level', 42, 'Open'),
    ('brightwavecivic@example.com', 'BrightWave Civic - Canberra', 'Community Program Coordinator', 'Coordinate delivery, service partner engagement, and reporting across funded community programs.', 'Bachelor''s', 'Program Coordination, Stakeholder Engagement, Reporting, Case Coordination, Workshop Facilitation, Community Outreach', 3, 'Hybrid', 'Canberra', 84000.00, 98000.00, 'Full-time', 'Mid-level', 35, 'Open'),
    ('brightwavecivic@example.com', 'BrightWave Civic - Canberra', 'Grants Reporting Specialist', 'Prepare compliance reports, audit-ready records, and grant performance summaries for public programs.', 'Bachelor''s', 'Compliance Reporting, Audit Preparation, Documentation Review, Excel, Policy Interpretation, Stakeholder Liaison', 3, 'Remote', 'Canberra', 82000.00, 96000.00, 'Full-time', 'Mid-level', 39, 'Open'),
    ('truenorthtravel@example.com', 'TrueNorth Travel - Melbourne', 'Customer Experience Designer', 'Design service journeys and digital interactions that improve booking, support, and loyalty experiences.', 'Bachelor''s', 'Figma, Journey Mapping, User Research, Prototyping, Accessibility, Service Design', 4, 'Hybrid', 'Melbourne', 92000.00, 108000.00, 'Full-time', 'Mid-level', 30, 'Open'),
    ('truenorthtravel@example.com', 'TrueNorth Travel - Melbourne', 'Digital Marketing Strategist', 'Lead multichannel campaign planning and optimise acquisition, retention, and travel content performance.', 'Bachelor''s', 'SEO, Google Analytics, Paid Search, CRM, Content Strategy, Campaign Planning', 4, 'Hybrid', 'Melbourne', 90000.00, 105000.00, 'Full-time', 'Mid-level', 34, 'Open'),
    ('luminabiotech@example.com', 'Lumina Biotech - Brisbane', 'Laboratory Quality Analyst', 'Maintain laboratory quality systems, support deviations, and review regulated documentation for biomedical operations.', 'Bachelor''s', 'Quality Control, Laboratory Documentation, CAPA, Data Review, Audit Support, Sample Tracking', 3, 'On-site', 'Brisbane', 90000.00, 104000.00, 'Full-time', 'Mid-level', 33, 'Open'),
    ('luminabiotech@example.com', 'Lumina Biotech - Brisbane', 'Biomedical Research Assistant', 'Prepare samples, support assay workflows, and maintain accurate data capture across research activities.', 'Bachelor''s', 'PCR, ELISA, Sample Preparation, Laboratory Support, Documentation Control, Data Entry', 2, 'On-site', 'Brisbane', 76000.00, 89000.00, 'Full-time', 'Entry-level', 27, 'Open'),
    ('redstoneproperty@example.com', 'RedStone Property - Sydney', 'BIM Coordinator', 'Coordinate project models, review documentation, and improve digital delivery standards for property developments.', 'Bachelor''s', 'Revit, AutoCAD, BIM Coordination, Construction Documentation, Clash Detection, Bluebeam', 4, 'Hybrid', 'Sydney', 102000.00, 124000.00, 'Full-time', 'Mid-level', 38, 'Open'),
    ('redstoneproperty@example.com', 'RedStone Property - Sydney', 'Property Operations Analyst', 'Support property reporting, contractor coordination, and operational planning for commercial assets.', 'Bachelor''s', 'Excel, Reporting, Documentation Control, Stakeholder Coordination, Risk Registers, Procurement Support', 3, 'Hybrid', 'Sydney', 88000.00, 102000.00, 'Full-time', 'Mid-level', 32, 'Open'),
    ('pulsepointfitness@example.com', 'PulsePoint Fitness - Perth', 'Member Experience Lead', 'Coordinate venue operations, coach frontline teams, and improve day-to-day member service outcomes.', 'Bachelor''s', 'Venue Operations, Staff Scheduling, Customer Service, POS Systems, Event Coordination, Inventory Ordering', 4, 'On-site', 'Perth', 78000.00, 90000.00, 'Full-time', 'Mid-level', 28, 'Open'),
    ('pulsepointfitness@example.com', 'PulsePoint Fitness - Perth', 'Performance Marketing Specialist', 'Manage growth campaigns, optimise digital acquisition, and analyse retention initiatives for fitness programs.', 'Bachelor''s', 'SEO, Google Analytics, Paid Search, CRM, Email Marketing, Campaign Analytics', 3, 'Hybrid', 'Perth', 84000.00, 98000.00, 'Full-time', 'Mid-level', 31, 'Open')
) AS v(email, company_info, job_title, job_description, required_education, required_skills, required_experience, work_mode, job_location, salary_min, salary_max, job_type, career_level, deadline_days, status)
INNER JOIN users u ON u.email = v.email
WHERE NOT EXISTS
(
    SELECT 1
    FROM job_postings jp
    WHERE jp.employer_id = u.id
      AND jp.job_title = v.job_title
);

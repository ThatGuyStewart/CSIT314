/**
 * Intelligent Talent Matching Platform – Main JavaScript
 */

(function () {
  'use strict';

  // ============================================================
  // MOBILE NAV TOGGLE
  // ============================================================
  const navToggle = document.querySelector('.nav-toggle');
  const navLinks  = document.querySelector('.nav-links');

  if (navToggle && navLinks) {
    navToggle.addEventListener('click', () => {
      navLinks.classList.toggle('open');
      navToggle.setAttribute(
        'aria-expanded',
        navLinks.classList.contains('open') ? 'true' : 'false'
      );
    });

    // Close nav when a link is clicked (mobile)
    navLinks.querySelectorAll('a').forEach(link => {
      link.addEventListener('click', () => {
        navLinks.classList.remove('open');
      });
    });

    // Close nav when clicking outside
    document.addEventListener('click', e => {
      if (!e.target.closest('.navbar')) {
        navLinks.classList.remove('open');
      }
    });
  }

  // ============================================================
  // CONFIRM BEFORE DELETE
  // ============================================================
  document.addEventListener('click', e => {
    const el = e.target.closest('[data-confirm]');
    if (!el) return;
    const msg = el.getAttribute('data-confirm') || 'Are you sure you want to delete this item? This action cannot be undone.';
    if (!confirm(msg)) {
      e.preventDefault();
      e.stopPropagation();
    }
  });

  // For delete forms triggered by buttons
  document.querySelectorAll('form[data-confirm]').forEach(form => {
    form.addEventListener('submit', e => {
      const msg = form.getAttribute('data-confirm') || 'Are you sure?';
      if (!confirm(msg)) {
        e.preventDefault();
      }
    });
  });

  // ============================================================
  // SALARY RANGE VALIDATION
  // ============================================================
  const salaryMinInput = document.querySelector('input[name="salary_min"], input[name="filter_salary_min"]');
  const salaryMaxInput = document.querySelector('input[name="salary_max"], input[name="filter_salary_max"]');

  function validateSalaryRange() {
    if (!salaryMinInput || !salaryMaxInput) return true;
    const min = parseFloat(salaryMinInput.value);
    const max = parseFloat(salaryMaxInput.value);
    if (min && max && min > max) {
      salaryMaxInput.setCustomValidity('Max salary must be greater than min salary.');
      salaryMaxInput.reportValidity();
      return false;
    }
    salaryMaxInput.setCustomValidity('');
    return true;
  }

  function formatMembershipStatus(status) {
    const normalized = (status || '').toLowerCase();
    return normalized ? `${normalized.charAt(0).toUpperCase()}${normalized.slice(1)}` : 'Free';
  }

  function getRecommendationLimit(status) {
    return (status || '').toLowerCase() === 'premium' ? 'Unlimited' : '10';
  }

  function escapeHtml(value) {
    return String(value ?? '')
      .replace(/&/g, '&amp;')
      .replace(/</g, '&lt;')
      .replace(/>/g, '&gt;')
      .replace(/"/g, '&quot;')
      .replace(/'/g, '&#39;');
  }

  function adminCandidateDetailsUrl(id) {
    return `/admin/candidate?id=${encodeURIComponent(id)}`;
  }

  function adminEmployerDetailsUrl(id) {
    return `/admin/employer?id=${encodeURIComponent(id)}`;
  }

  function adminJobDetailsUrl(id) {
    return `/admin/job?id=${encodeURIComponent(id)}`;
  }

  function jobDetailsUrl(id) {
    return `/candidate/job?id=${encodeURIComponent(id)}`;
  }

  function employerCandidateDetailsUrl(id) {
    return `/employer/candidate?id=${encodeURIComponent(id)}`;
  }

  function employerCandidateDetailsForJobUrl(candidateId, jobId) {
    const params = new URLSearchParams();
    params.set('id', candidateId);
    if (jobId) {
      params.set('job_id', jobId);
    }

    return `/employer/candidate?${params.toString()}`;
  }

  function getAdminUserDetailsUrl(user) {
    if (!user || !user.id) return '';
    if (user.role === 'candidate') return adminCandidateDetailsUrl(user.id);
    if (user.role === 'employer') return adminEmployerDetailsUrl(user.id);
    return '';
  }

  function renderSkillBadges(skills) {
    return String(skills || '')
      .split(',')
      .map(skill => skill.trim())
      .filter(Boolean)
      .map(skill => `<span class="skill-tag">${escapeHtml(skill)}</span>`)
      .join('');
  }

  function runPageInitializer(pathname, initializersByPath) {
    const initializer = initializersByPath[pathname.toLowerCase()];
    if (typeof initializer === 'function') {
      initializer();
    }
  }

  async function loadEmployerApplicants() {
    const groupsContainer = document.querySelector('#applicants-groups');
    if (!groupsContainer) return;

    const totalJobsCount = document.querySelector('#jobs-with-applicants-count');

    try {
      const response = await fetch('/api/employer/applicants', { credentials: 'same-origin' });
      const data = await response.json().catch(() => ({}));
      if (!response.ok) {
        groupsContainer.innerHTML = '<div class="card">Unable to load applicants.</div>';
        return;
      }

      const jobs = data.jobs || [];
      if (totalJobsCount) {
        totalJobsCount.textContent = String(jobs.length);
      }

      groupsContainer.innerHTML = jobs.length
        ? jobs.map(job => {
            const applicantsMarkup = Array.isArray(job.applicants) && job.applicants.length
              ? job.applicants.map(applicant => `
                  <a class="card" href="${employerCandidateDetailsForJobUrl(applicant.id, job.jobId)}" style="display:block; text-decoration:none; color:inherit;">
                    <div style="display:flex; justify-content:space-between; gap:12px; align-items:flex-start; flex-wrap:wrap;">
                      <div>
                        <div class="card-title">${escapeHtml(applicant.fullName || 'Candidate')}</div>
                        <div style="color:var(--text-muted); margin-bottom:8px;">${escapeHtml(applicant.major || 'Field not listed')}</div>
                      </div>
                      <span class="badge">Applicant</span>
                    </div>
                    <div style="font-size:0.9rem; margin-bottom:8px;">${escapeHtml(applicant.skills || 'No skills listed')}</div>
                    <div style="font-size:0.85rem; color:var(--text-muted);">${escapeHtml(applicant.preferredLocation || 'Location not listed')} • ${escapeHtml(applicant.preferredWorkMode || 'Mode not listed')} • ${escapeHtml(applicant.experienceText || 'Experience not listed')}</div>
                  </a>
                `).join('')
              : '<div class="card">No applicants yet for this job.</div>';

            return `
              <section class="card">
                <div style="display:flex; justify-content:space-between; gap:16px; align-items:flex-start; flex-wrap:wrap; margin-bottom:16px;">
                  <div>
                    <div class="card-title">${escapeHtml(job.jobTitle || 'Untitled job')}</div>
                    <div style="color:var(--text-muted); margin-top:6px;">${escapeHtml(job.location || 'Location not listed')} • ${escapeHtml(job.workMode || 'Mode not listed')} • ${escapeHtml(job.status || 'Unknown')}</div>
                  </div>
                  <span class="badge">${escapeHtml(String(job.applicantCount || 0))} applicants</span>
                </div>
                <div style="display:flex; flex-direction:column; gap:12px;">${applicantsMarkup}</div>
              </section>
            `;
          }).join('')
        : '<div class="card">No job postings found yet.</div>';
    } catch {
      groupsContainer.innerHTML = '<div class="card">Unable to load applicants.</div>';
    }
  }

  async function loadEmployerEditJob() {
    const form = document.querySelector('#edit-job-form');
    if (!form) return;

    const errorBox = document.querySelector('#error-message');
    const successBox = document.querySelector('#success-message');
    const statusBadge = document.querySelector('#job-status-badge');
    const params = new URLSearchParams(window.location.search);
    const jobId = params.get('id');
    const requiredSkillsInput = document.querySelector('#required-skills-input');
    const requiredSkillsPreview = document.querySelector('#required-skills-preview');

    const setMessage = (element, message) => {
      if (!element) return;
      element.textContent = message;
      element.hidden = !message;
    };

    const renderSkills = (value) => {
      if (!requiredSkillsPreview) return;
      requiredSkillsPreview.innerHTML = renderSkillBadges(value);
    };

    if (!jobId) {
      setMessage(errorBox, 'Missing job id.');
      return;
    }

    try {
      const response = await fetch(`/api/employer/edit-job?id=${encodeURIComponent(jobId)}`, { credentials: 'same-origin' });
      const data = await response.json().catch(() => ({}));
      if (!response.ok || !data.job) {
        setMessage(errorBox, data.error || 'Unable to load job details.');
        return;
      }

      const job = data.job;
      const setFieldValue = (name, value) => {
        const field = form.querySelector(`[name="${name}"]`);
        if (field) {
          field.value = value ?? '';
        }
      };

      setFieldValue('job_id', job.id);
      setFieldValue('job_title', job.title);
      setFieldValue('job_description', job.jobDescription);
      setFieldValue('required_skills', job.requiredSkills);
      setFieldValue('required_education', job.requiredEducation || 'Any');
      setFieldValue('required_experience', Number(job.requiredExperience || 0));
      setFieldValue('career_level', job.careerLevel || 'Entry-level');
      setFieldValue('job_type', job.type || 'Full-time');
      setFieldValue('work_mode', job.workMode || 'On-site');
      setFieldValue('job_location', job.location);
      setFieldValue('salary_min', job.salaryMin);
      setFieldValue('salary_max', job.salaryMax);
      setFieldValue('application_deadline', job.deadline);
      setFieldValue('status', job.status || 'Open');

      if (statusBadge) {
        statusBadge.textContent = job.status || 'Open';
      }

      renderSkills(job.requiredSkills || '');
      setMessage(errorBox, '');
      setMessage(successBox, '');
    } catch {
      setMessage(errorBox, 'Unable to load job details.');
      return;
    }

    if (requiredSkillsInput) {
      requiredSkillsInput.addEventListener('input', () => {
        renderSkills(requiredSkillsInput.value);
      });
    }

    form.addEventListener('submit', async event => {
      event.preventDefault();
      if (!validateSalaryRange()) return;

      const formData = new FormData(form);
      const payload = {
        jobId: Number(formData.get('job_id') || 0),
        job_title: String(formData.get('job_title') || ''),
        job_description: String(formData.get('job_description') || ''),
        required_skills: String(formData.get('required_skills') || ''),
        required_education: String(formData.get('required_education') || 'Any'),
        required_experience: Number(formData.get('required_experience') || 0),
        career_level: String(formData.get('career_level') || 'Entry-level'),
        job_type: String(formData.get('job_type') || 'Full-time'),
        work_mode: String(formData.get('work_mode') || 'On-site'),
        job_location: String(formData.get('job_location') || ''),
        salary_min: String(formData.get('salary_min') || ''),
        salary_max: String(formData.get('salary_max') || ''),
        application_deadline: String(formData.get('application_deadline') || ''),
        status: String(formData.get('status') || 'Open')
      };

      try {
        const response = await fetch('/api/employer/edit-job', {
          method: 'POST',
          headers: {
            'Content-Type': 'application/json'
          },
          credentials: 'same-origin',
          body: JSON.stringify(payload)
        });

        const data = await response.json().catch(() => ({}));
        if (!response.ok) {
          setMessage(successBox, '');
          setMessage(errorBox, data.error || 'Unable to save job changes.');
          return;
        }

        window.location.href = '/employer/manage-jobs?success=' + encodeURIComponent('Job updated successfully.');
      } catch {
        setMessage(successBox, '');
        setMessage(errorBox, 'Unable to save job changes.');
      }
    });
  }

  function applyPlanPresentation(options) {
    const status = formatMembershipStatus(options.status);
    const isPremium = status.toLowerCase() === 'premium';
    const limit = getRecommendationLimit(status);
    const subject = options.subject || 'job';

    const setText = (selector, value) => {
      const el = document.querySelector(selector);
      if (el) el.textContent = value;
    };

    if (options.badgeSelector) setText(options.badgeSelector, status);
    if (options.nameSelector) setText(options.nameSelector, `${status} Plan`);
    if (options.limitSelector) setText(options.limitSelector, limit);
    if (options.titleSelector) setText(options.titleSelector, `Current Plan: ${status}`);
    if (options.copySelector) {
      setText(options.copySelector, isPremium
        ? `You are on the Premium plan. You can view unlimited ${subject} recommendations.`
        : `You are on the Free plan. You can view up to 10 ${subject} recommendations.`);
    }

    const freeMarker = options.freeMarkerSelector ? document.querySelector(options.freeMarkerSelector) : null;
    const premiumButton = options.premiumButtonSelector ? document.querySelector(options.premiumButtonSelector) : null;
    const primaryAction = options.primaryActionSelector ? document.querySelector(options.primaryActionSelector) : null;
    if (freeMarker) freeMarker.textContent = isPremium ? '⬇ Downgrade to Free Plan' : 'Current Plan';
    if (freeMarker) freeMarker.dataset.membershipTarget = 'free';
    if (freeMarker) freeMarker.disabled = !isPremium;
    if (freeMarker) freeMarker.style.cursor = isPremium ? 'pointer' : 'default';
    if (freeMarker) freeMarker.className = isPremium ? 'btn btn-secondary btn-full' : 'badge badge-open';
    if (freeMarker) freeMarker.style.width = '100%';
    if (freeMarker) freeMarker.style.justifyContent = 'center';
    if (freeMarker) freeMarker.style.padding = '10px';
    if (freeMarker) freeMarker.style.border = isPremium ? '1px solid #000' : 'none';
    if (freeMarker) freeMarker.style.fontWeight = isPremium ? '700' : '';
    if (premiumButton) premiumButton.textContent = isPremium ? 'Current Plan' : '⭐ Upgrade Now';
    if (premiumButton) premiumButton.dataset.membershipTarget = 'premium';
    if (premiumButton) premiumButton.disabled = isPremium;
    if (premiumButton) premiumButton.className = isPremium ? 'badge badge-open' : 'btn btn-full';
    if (premiumButton) premiumButton.style.width = '100%';
    if (premiumButton) premiumButton.style.justifyContent = 'center';
    if (premiumButton) premiumButton.style.padding = '10px';
    if (premiumButton) premiumButton.style.border = 'none';
    if (premiumButton) premiumButton.style.cursor = isPremium ? 'default' : 'pointer';
    if (premiumButton) premiumButton.style.background = isPremium ? '' : '#fff';
    if (premiumButton) premiumButton.style.color = isPremium ? '' : 'var(--navy-primary)';
    if (premiumButton) premiumButton.style.fontWeight = '700';
    if (primaryAction) primaryAction.textContent = isPremium ? '⬇ Downgrade to Free Plan' : '⭐ Upgrade to Premium';
    if (primaryAction) primaryAction.dataset.membershipTarget = isPremium ? 'free' : 'premium';
    if (primaryAction) primaryAction.disabled = false;
    if (primaryAction) primaryAction.style.border = isPremium ? '1px solid #000' : '';
  }

  async function updateMembershipStatus(endpoint, membershipStatus) {
    const response = await fetch(endpoint, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json'
      },
      credentials: 'same-origin',
      body: JSON.stringify({ membership_status: membershipStatus })
    });

    const data = await response.json().catch(() => ({}));
    if (!response.ok) {
      throw new Error(data.error || 'Failed to update membership.');
    }

    return data;
  }

  function buildMatchReasonsMarkup(reasons, emptyMessage) {
    return Array.isArray(reasons) && reasons.length
      ? reasons.map(reason => `<li>${escapeHtml(reason)}</li>`).join('')
      : `<li>${escapeHtml(emptyMessage)}</li>`;
  }

  function clampMatchScore(score) {
    const numericScore = Number(score);
    if (!Number.isFinite(numericScore)) return 0;
    return Math.max(0, Math.min(100, numericScore));
  }

  function getMatchColor(score) {
    const clampedScore = clampMatchScore(score);

    if (clampedScore <= 50) {
      const hue = (clampedScore / 50) * 60;
      return `hsl(${hue}, 72%, 45%)`;
    }

    const progress = (clampedScore - 50) / 50;
    const hue = 60 + ((141 - 60) * progress);
    const saturation = 72 - ((72 - 46) * progress);
    const lightness = 45 - ((45 - 34) * progress);
    return `hsl(${hue}, ${saturation}%, ${lightness}%)`;
  }

  function getMatchBarStyle(score) {
    const color = getMatchColor(score);
    return `background: linear-gradient(90deg, ${color}, ${color});`;
  }

  function applyScoreCircleColor(selector, score) {
    const el = document.querySelector(selector);
    if (el) {
      el.style.setProperty('--score-color', getMatchColor(score));
    }
  }

  function applyMatchBarColor(selector, score) {
    const el = document.querySelector(selector);
    if (el) {
      el.style.background = getMatchColor(score);
    }
  }

  function renderCandidateRecommendedJobCard(job) {
    const matchScore = clampMatchScore(job.matchScore);
    return `
            <a class="card recommended-job-card" href="${jobDetailsUrl(job.id)}" style="display:block; text-decoration:none; color:inherit;">
              <div style="display:flex; justify-content:space-between; align-items:flex-start; gap:16px; margin-bottom:16px;">
                <div>
                  <div class="card-title">${escapeHtml(job.title)}</div>
                  <div style="color:var(--text-muted); margin-bottom:8px;">${escapeHtml(job.company || 'Company not listed')}</div>
                  <div style="font-size:0.9rem; color:var(--text-muted);">${escapeHtml(job.location || 'Location not listed')} • ${escapeHtml(job.workMode || 'Mode not listed')} • ${escapeHtml(job.type || 'Type not listed')}</div>
                </div>
                <div style="text-align:right; min-width:96px;">
                  <div class="score-circle score-circle-sm" style="--score-color:${getMatchColor(matchScore)};">${matchScore}%</div>
                  <div style="font-size:0.8rem; color:var(--text-muted); margin-top:6px;">Match</div>
                </div>
              </div>
              <div style="margin-bottom:12px; font-weight:600; color:var(--navy-primary);">${escapeHtml(job.salaryRange || 'Salary not listed')}</div>
              <div class="match-bar-wrap" style="margin-bottom:14px;">
                <div class="match-bar" style="width:${matchScore}%; ${getMatchBarStyle(matchScore)}"></div>
              </div>
            </a>
          `;
  }

  function renderCandidateApplicationCard(job) {
    return `
      <a class="card recommended-job-card" href="${jobDetailsUrl(job.id)}" style="display:block; text-decoration:none; color:inherit;">
        <div style="display:flex; justify-content:space-between; align-items:flex-start; gap:16px; margin-bottom:16px;">
          <div>
            <div class="card-title">${escapeHtml(job.title || 'Untitled job')}</div>
            <div style="color:var(--text-muted); margin-bottom:8px;">${escapeHtml(job.company || 'Company not listed')}</div>
            <div style="font-size:0.9rem; color:var(--text-muted);">${escapeHtml(job.location || 'Location not listed')} • ${escapeHtml(job.workMode || 'Mode not listed')} • ${escapeHtml(job.type || 'Type not listed')}</div>
          </div>
          <span class="badge">Applied</span>
        </div>
        <div style="margin-bottom:12px; font-weight:600; color:var(--navy-primary);">${escapeHtml(job.salaryRange || 'Salary not listed')}</div>
      </a>
    `;
  }

  function renderCandidateBrowseJobCard(job) {
    return `
            <a class="card recommended-job-card" href="${jobDetailsUrl(job.id)}" style="display:block; text-decoration:none; color:inherit;">
              <div style="display:flex; justify-content:space-between; align-items:flex-start; gap:16px; margin-bottom:16px;">
                <div>
                  <div class="card-title">${escapeHtml(job.title)}</div>
                  <div style="color:var(--text-muted); margin-bottom:8px;">${escapeHtml(job.company || 'Company not listed')}</div>
                  <div style="font-size:0.9rem; color:var(--text-muted);">${escapeHtml(job.location || 'Location not listed')} • ${escapeHtml(job.workMode || 'Mode not listed')} • ${escapeHtml(job.type || 'Type not listed')}</div>
                </div>
              </div>
              <div style="margin-bottom:12px; font-weight:600; color:var(--navy-primary);">${escapeHtml(job.salaryRange || 'Salary not listed')}</div>
            </a>
          `;
  }

  function renderEmployerBrowseCandidateCard(candidate) {
    const candidateId = candidate.candidateId ?? candidate.id;

    return `
      <a class="card" href="${employerCandidateDetailsUrl(candidateId)}" style="display:block; text-decoration:none; color:inherit;">
        <div style="display:flex; justify-content:space-between; gap:12px; align-items:flex-start;">
          <div>
            <div class="card-title">${escapeHtml(candidate.fullName)}</div>
            <div style="color:var(--text-muted); margin-bottom:8px;">${escapeHtml(candidate.major || 'Field not listed')}</div>
          </div>
        </div>
        <div style="font-size:0.9rem; margin-bottom:8px;">${escapeHtml(candidate.skills || 'No skills listed')}</div>
        <div style="font-size:0.85rem; color:var(--text-muted); margin-bottom:8px;">${escapeHtml(candidate.preferredLocation || 'Location not listed')} • ${escapeHtml(candidate.preferredWorkMode || 'Mode not listed')} • ${escapeHtml(candidate.experienceText || 'Experience not listed')}</div>
      </a>
    `;
  }

  async function loadRecommendedJobsPage() {
    const grid = document.querySelector('#recommendations-grid');
    if (!grid) return;

    try {
      const response = await fetch('/api/candidate/recommended-jobs', { credentials: 'same-origin' });
      const data = await response.json().catch(() => ({}));
      if (!response.ok) return;

      const jobs = data.jobs || [];
      const membershipStatus = data.membershipStatus || data.membership_status || 'free';
      const totalCountEl = document.querySelector('#total-count');
      const visibleCountEl = document.querySelector('#visible-count');
      const membershipBadge = document.querySelector('#membership-badge');

      if (membershipBadge) {
        membershipBadge.textContent = formatMembershipStatus(membershipStatus);
      }

      if (totalCountEl) totalCountEl.textContent = String(jobs.length);
      if (visibleCountEl) visibleCountEl.textContent = String(jobs.length);

      grid.innerHTML = jobs.length
        ? jobs.map(renderCandidateRecommendedJobCard).join('')
        : '<div class="card">No recommended jobs available right now.</div>';
    } catch {
      grid.innerHTML = '<div class="card">Unable to load recommended jobs.</div>';
    }
  }

  async function loadCandidateApplications() {
    const grid = document.querySelector('#applications-grid');
    if (!grid) return;

    const shownCount = document.querySelector('#applications-shown-count');
    const totalCount = document.querySelector('#applications-total-count');

    try {
      const response = await fetch('/api/candidate/applications', { credentials: 'same-origin' });
      const data = await response.json().catch(() => ({}));
      if (!response.ok) {
        grid.innerHTML = '<div class="card">Unable to load applications.</div>';
        return;
      }

      const jobs = data.jobs || [];
      if (shownCount) shownCount.textContent = String(jobs.length);
      if (totalCount) totalCount.textContent = String(jobs.length);

      grid.innerHTML = jobs.length
        ? jobs.map(renderCandidateApplicationCard).join('')
        : '<div class="card">You have not applied to any jobs yet.</div>';
    } catch {
      grid.innerHTML = '<div class="card">Unable to load applications.</div>';
    }
  }

  async function loadCandidateDashboard() {
    const recommendedCount = document.querySelector('#recommendedCount');
    if (!recommendedCount) return;

    try {
      const response = await fetch('/api/candidate/dashboard', { credentials: 'same-origin' });
      const data = await response.json().catch(() => ({}));
      if (!response.ok) return;

      const setText = (selector, value) => {
        const el = document.querySelector(selector);
        if (el) el.textContent = value;
      };

      setText('#recommendedCount', data.recommendedCount ?? 0);
      setText('#profileComplete', `${data.profileComplete ?? 0}%`);
      setText('#jobCount', data.jobCount ?? 0);

      const completionBar = document.querySelector('.completion-bar');
      if (completionBar) {
        completionBar.style.width = `${data.profileComplete ?? 0}%`;
      }

      const completionText = document.querySelector('.completion-bar-wrap + div span');
      if (completionText) {
        completionText.textContent = `${data.profileComplete ?? 0}% complete`;
      }

      applyPlanPresentation({
        status: data.membershipStatus || 'free',
        subject: 'job',
        badgeSelector: '#candidate-plan-badge',
        limitSelector: '#candidate-rec-limit',
        nameSelector: '#candidate-membership-name',
        copySelector: '#candidate-membership-copy'
      });

      const topRecommendations = document.querySelector('#top-recommendations');
      if (topRecommendations) {
        const jobs = data.topRecommendations || [];
        topRecommendations.innerHTML = jobs.length
          ? jobs.map(renderCandidateRecommendedJobCard).join('')
          : '<div class="card">No recommendations available yet.</div>';
      }
    } catch {
    }
  }

  async function loadCandidateJobs() {
    const grid = document.querySelector('#jobs-grid');
    if (!grid) return;

    const form = document.querySelector('form[action="/candidate/jobs"]');
    const params = new URLSearchParams(window.location.search);
    const shownCount = document.querySelector('#shownCount');
    const totalCount = document.querySelector('#totalCount');
    const fieldNames = ['keyword', 'location', 'work_mode', 'job_type', 'career_level', 'salary_min', 'salary_max'];

    fieldNames.forEach(name => {
      const field = form?.querySelector(`[name="${name}"]`);
      if (field && params.has(name)) {
        field.value = params.get(name) || '';
      }
    });

    const query = params.toString();

    try {
      const response = await fetch(`/api/candidate/jobs${query ? `?${query}` : ''}`, { credentials: 'same-origin' });
      const data = await response.json().catch(() => ({}));
      if (!response.ok) {
        grid.innerHTML = '<div class="card">Unable to load jobs.</div>';
        return;
      }

      const jobs = data.jobs || [];
      if (shownCount) shownCount.textContent = String(jobs.length);
      if (totalCount) totalCount.textContent = String(jobs.length);

      grid.innerHTML = jobs.length
        ? jobs.map(renderCandidateBrowseJobCard).join('')
        : '<div class="card">No jobs found.</div>';
    } catch {
      grid.innerHTML = '<div class="card">Unable to load jobs.</div>';
    }
  }

  async function loadEmployerDashboard() {
    const activeCount = document.querySelector('#activeCount');
    if (!activeCount) return;

    try {
      const response = await fetch('/api/employer/dashboard', { credentials: 'same-origin' });
      const data = await response.json().catch(() => ({}));
      if (!response.ok) return;

      const setText = (selector, value) => {
        const el = document.querySelector(selector);
        if (el) el.textContent = value;
      };

      setText('#activeCount', data.activeCount ?? 0);
      setText('#totalPosts', data.totalPosts ?? 0);
      setText('#totalCandidates', data.totalCandidates ?? 0);

      applyPlanPresentation({
        status: data.membershipStatus || 'free',
        subject: 'candidate',
        badgeSelector: '#employer-plan-badge',
        limitSelector: '#employer-rec-limit',
        nameSelector: '#employer-membership-name',
        copySelector: '#employer-membership-copy'
      });

      const topCandidates = document.querySelector('#top-candidates');
      if (topCandidates) {
        const candidates = data.topCandidates || [];
        topCandidates.innerHTML = candidates.length
          ? candidates.map(candidate => renderEmployerRecommendationCard(candidate, { showMatchedJobs: true })).join('')
          : '<div class="card">No recommendations available yet.</div>';
      }
    } catch {
      const topCandidates = document.querySelector('#top-candidates');
      if (topCandidates) {
        topCandidates.innerHTML = '<div class="card">Unable to load top candidates.</div>';
      }
    }
  }

  async function loadEmployerRecommendedCandidatesPage() {
    const grid = document.querySelector('#recommendations-grid');
    if (!grid) return;

    const params = new URLSearchParams(window.location.search);
    const selectedJobId = params.get('job_id') || '';
    const jobSelect = document.querySelector('select[name="job_id"]');
    const totalCountEl = document.querySelector('#total-count');
    const visibleCountEl = document.querySelector('#visible-count');
    const membershipBadge = document.querySelector('.page-header .badge');

    try {
      const jobsResponse = await fetch('/api/employer/jobs', { credentials: 'same-origin' });
      const jobsData = await jobsResponse.json().catch(() => ({}));
      const jobs = jobsResponse.ok ? (jobsData.jobs || []) : [];

      if (jobSelect) {
        const optionsMarkup = jobs.map(job => {
          const isSelected = String(job.id) === selectedJobId ? ' selected' : '';
          return `<option value="${escapeHtml(job.id)}"${isSelected}>${escapeHtml(job.title || 'Untitled job')}</option>`;
        }).join('');

        jobSelect.innerHTML = `<option value="">Select a job…</option>${optionsMarkup}`;
      }

      const dashboardResponse = await fetch('/api/employer/dashboard', { credentials: 'same-origin' });
      const dashboardData = await dashboardResponse.json().catch(() => ({}));
      const membershipStatus = dashboardResponse.ok ? (dashboardData.membershipStatus || dashboardData.membership_status || 'free') : 'free';
      if (membershipBadge) {
        membershipBadge.textContent = formatMembershipStatus(membershipStatus);
      }

        const recommendationsUrl = selectedJobId
            ? `/api/employer/recommendations?job_id=${encodeURIComponent(selectedJobId)}`
            : '/api/employer/recommendations';

          const response = await fetch(recommendationsUrl, { credentials: 'same-origin' });
      const data = await response.json().catch(() => ({}));
      if (!response.ok) {
        grid.innerHTML = '<div class="card">Unable to load recommended candidates.</div>';
        return;
      }

      const candidates = data.candidates || data.recommendations || data.topCandidates || [];
      if (totalCountEl) totalCountEl.textContent = String(candidates.length);
      if (visibleCountEl) visibleCountEl.textContent = String(candidates.length);

      grid.innerHTML = candidates.length
            ? candidates.map(candidate => renderEmployerRecommendationCard(candidate, { showMatchedJobs: !selectedJobId })).join('')
            : (selectedJobId
                ? '<div class="card">No matched candidates found for this job.</div>'
                : '<div class="card">No recommended candidates available yet.</div>');
    } catch {
      grid.innerHTML = '<div class="card">Unable to load recommended candidates.</div>';
    }
  }

  async function loadEmployerJobs() {
    const jobsList = document.querySelector('#jobs-list');
    if (!jobsList) return;

    const successBox = document.querySelector('#success-message');
    const errorBox = document.querySelector('#error-message');

    try {
      const response = await fetch('/api/employer/jobs', { credentials: 'same-origin' });
      const data = await response.json().catch(() => ({}));
      if (!response.ok) {
        jobsList.innerHTML = '<div class="card">Unable to load job postings.</div>';
        return;
      }

      const jobs = data.jobs || [];
      const jobCount = document.querySelector('#job-count');
      if (jobCount) {
        jobCount.textContent = String(jobs.length);
      }

      if (successBox) {
        const successMessage = new URLSearchParams(window.location.search).get('success');
        if (successMessage) {
          successBox.textContent = successMessage;
          successBox.hidden = false;
        }
      }

      if (errorBox) {
        errorBox.hidden = true;
      }

      jobsList.innerHTML = jobs.length
        ? jobs.map(job => `
            <div class="card">
              <div style="display:flex; justify-content:space-between; align-items:flex-start; gap:16px; flex-wrap:wrap;">
                <div>
                  <div class="card-title">${escapeHtml(job.title || 'Untitled job')}</div>
                  <div style="color:var(--text-muted); margin-bottom:8px;">${escapeHtml(job.location || 'Location not listed')} • ${escapeHtml(job.workMode || 'Mode not listed')} • ${escapeHtml(job.type || 'Type not listed')}</div>
                  <div style="font-size:0.9rem; color:var(--text-muted);">${escapeHtml(job.salaryRange || 'Salary not listed')}</div>
                </div>
                <div style="display:flex; gap:10px; flex-wrap:wrap;">
                  <a href="/employer/edit-job?id=${encodeURIComponent(job.id)}" class="btn btn-secondary btn-sm">Edit</a>
                </div>
              </div>
            </div>
          `).join('')
        : '<div class="card">No job postings yet.</div>';
    } catch {
      jobsList.innerHTML = '<div class="card">Unable to load job postings.</div>';
    }
  }

  async function loadEmployerCandidates() {
    const grid = document.querySelector('#candidates-grid');
    if (!grid) return;

    const form = document.querySelector('#employer-candidates-form');
    const params = new URLSearchParams(window.location.search);
    const shownCount = document.querySelector('#shownCount');
    const totalCount = document.querySelector('#totalCount');

    const fieldNames = ['keyword', 'skills', 'education', 'years_experience', 'preferred_work_mode', 'preferred_location'];
    fieldNames.forEach(name => {
      const field = form?.querySelector(`[name="${name}"]`);
      if (field && params.has(name)) {
        field.value = params.get(name) || '';
      }
    });

    const query = params.toString();

    try {
      const response = await fetch(`/api/employer/candidates${query ? `?${query}` : ''}`, { credentials: 'same-origin' });
      const data = await response.json().catch(() => ({}));
      if (!response.ok) {
        grid.innerHTML = '<div class="card">Unable to load candidates.</div>';
        return;
      }

      const candidates = data.candidates || [];
      if (shownCount) shownCount.textContent = String(candidates.length);
      if (totalCount) totalCount.textContent = String(candidates.length);

      grid.innerHTML = candidates.length
        ? candidates.map(renderEmployerBrowseCandidateCard).join('')
        : '<div class="card">No candidates found.</div>';
    } catch {
      grid.innerHTML = '<div class="card">Unable to load candidates.</div>';
    }
  }

  async function loadEmployerCandidateDetails() {
    const name = document.querySelector('#candidate-name');
    if (!name) return;

    const params = new URLSearchParams(window.location.search);
    const candidateId = params.get('id');
    if (!candidateId) return;

    const form = document.querySelector('form[action="/employer/candidate_details"]');
    const hiddenId = form?.querySelector('input[name="id"]');
    const jobSelect = form?.querySelector('select[name="job_id"]');
    const selectedJobId = params.get('job_id') || '';

    if (hiddenId) {
      hiddenId.value = candidateId;
    }

    try {
      const jobsResponse = await fetch('/api/employer/jobs', { credentials: 'same-origin' });
      const jobsData = await jobsResponse.json().catch(() => ({}));
      const jobs = jobsResponse.ok ? (jobsData.jobs || []) : [];

      if (jobSelect) {
        const optionsMarkup = jobs.map(job => {
          const isSelected = String(job.id) === selectedJobId ? ' selected' : '';
          return `<option value="${escapeHtml(job.id)}"${isSelected}>${escapeHtml(job.title || 'Untitled job')}</option>`;
        }).join('');

        jobSelect.innerHTML = `<option value="">Select a job posting…</option>${optionsMarkup}`;
      }

      const requestUrl = selectedJobId
        ? `/api/employer/candidate?id=${encodeURIComponent(candidateId)}&job_id=${encodeURIComponent(selectedJobId)}`
        : `/api/employer/candidate?id=${encodeURIComponent(candidateId)}`;

      const response = await fetch(requestUrl, { credentials: 'same-origin' });
      const data = await response.json().catch(() => ({}));
      if (!response.ok || !data.candidate) return;

      const candidate = data.candidate;
        const matchLabelPrefix = selectedJobId ? 'Match score for' : 'Best match job';
      const setText = (selector, value, fallback = '') => {
        const el = document.querySelector(selector);
        if (el) el.textContent = value || fallback;
      };

      setText('#candidate-name', candidate.fullName, 'Candidate');
      setText('#candidate-summary-meta', `${candidate.education || 'Education not listed'} · ${candidate.experienceText || 'Experience not listed'}`);
      setText('#about-text', candidate.summary, 'No summary provided.');
      setText('#work-experience', candidate.workExperience, 'No work experience listed.');
      setText('#education', candidate.education, 'Not provided');
      setText('#major', candidate.major, 'Not provided');
      setText('#experience', candidate.experienceText, 'Not provided');
      setText('#employment-type', candidate.employmentType, 'Not provided');
      setText('#work-mode', candidate.preferredWorkMode, 'Not provided');
      setText('#preferred-location', candidate.preferredLocation, 'Not provided');
      setText('#expected-salary', candidate.expectedSalary, 'Not provided');
      setText('#contact-info', candidate.contactInfo, 'Not provided');
      setText('#member-since', candidate.createdAt, 'Not provided');
      setText('#candidate-work-mode-badge', candidate.preferredWorkMode, 'Not provided');
          setText('#matched-job-label', candidate.matchedJobTitle ? `${matchLabelPrefix}: ${candidate.matchedJobTitle}` : `${matchLabelPrefix}: Not available`);

      const skillsContainer = document.querySelector('#skills-container');
      if (skillsContainer) {
        skillsContainer.innerHTML = renderSkillBadges(candidate.skills) || '<span style="color:var(--text-muted);">No skills listed.</span>';
      }

      const portfolioLink = document.querySelector('#candidate-portfolio-link');
      if (portfolioLink) {
        if (candidate.portfolioUrl) {
          portfolioLink.href = candidate.portfolioUrl;
          portfolioLink.hidden = false;
        } else {
          portfolioLink.hidden = true;
        }
      }

      const matchScore = clampMatchScore(candidate.matchScore);
      setText('#candidate-match-score-circle', `${matchScore}%`);
      setText('#candidate-match-score-text', `${matchScore}%`);
      const matchScoreBar = document.querySelector('#candidate-match-score-bar');
      if (matchScoreBar) {
        matchScoreBar.style.width = `${matchScore}%`;
        matchScoreBar.removeAttribute('data-width');
      }
      applyScoreCircleColor('#candidate-match-score-circle', matchScore);
      applyMatchBarColor('#candidate-match-score-bar', matchScore);

      const highlights = document.querySelector('#match-highlights');
      if (highlights) {
        highlights.innerHTML = buildMatchReasonsMarkup(candidate.matchReasons, 'No personalized match highlights available.');
      }
    } catch {
    }
  }

  async function loadCandidateJobDetails() {
    const title = document.querySelector('#job-title');
    if (!title) return;

    const params = new URLSearchParams(window.location.search);
    const jobId = params.get('id');
    if (!jobId) return;

    try {
      const response = await fetch(`/api/candidate/job?id=${encodeURIComponent(jobId)}`, { credentials: 'same-origin' });
      const data = await response.json().catch(() => ({}));
      if (!response.ok || !data.job) return;

      const job = data.job;
      const setText = (selector, value, fallback = '') => {
        const el = document.querySelector(selector);
        if (el) el.textContent = value || fallback;
      };

      setText('#job-title', job.title, 'Job Title');
      setText('#company-name', `🏢 ${job.company || 'Company'}`);
      setText('#job-status', job.status, 'Unknown');
      setText('#job-location', `📍 ${job.location || 'Location not listed'}`);
      setText('#job-workmode', job.workMode, 'Mode not listed');
      setText('#job-type', `💼 ${job.type || 'Type not listed'}`);
      setText('#job-level', `📊 ${job.careerLevel || 'Level not listed'}`);
      setText('#job-salary', `💰 ${job.salaryRange || 'Salary not listed'}`);
      setText('#job-description', job.jobDescription, 'No job description available.');
      setText('#company-description', job.companyDescription, 'No company description available.');
      setText('#required-education', job.requiredEducation, 'Not specified');
      setText('#required-experience', job.requiredExperience ? `${job.requiredExperience} years` : 'Not specified');
      setText('#detail-job-type', job.type, 'Not specified');
      setText('#detail-career-level', job.careerLevel, 'Not specified');
      setText('#application-deadline', job.deadline, 'Not specified');
      setText('#detail-salary-range', job.salaryRange, 'Not specified');

      const skillsContainer = document.querySelector('#required-skills');
      if (skillsContainer) {
        skillsContainer.innerHTML = renderSkillBadges(job.requiredSkills) || '<span style="color:var(--text-muted);">No skills listed.</span>';
      }

      const matchScore = clampMatchScore(job.matchScore);
      setText('#match-score-circle', `${matchScore}%`);
      setText('#match-score-text', `${matchScore}%`);
      const matchScoreBar = document.querySelector('#match-score-bar');
      if (matchScoreBar) {
        matchScoreBar.style.width = `${matchScore}%`;
        matchScoreBar.removeAttribute('data-width');
      }
      applyScoreCircleColor('#match-score-circle', matchScore);
      applyMatchBarColor('#match-score-bar', matchScore);

      const reasons = document.querySelector('#match-reasons');
      if (reasons) {
        reasons.innerHTML = buildMatchReasonsMarkup(job.matchReasons, 'No personalized match reasons available.');
      }

      const companyWebsiteLink = document.querySelector('.card a.btn.btn-secondary.btn-sm');
      if (companyWebsiteLink) {
        if (job.companyWebsiteUrl) {
          companyWebsiteLink.href = job.companyWebsiteUrl;
          companyWebsiteLink.hidden = false;
        } else {
          companyWebsiteLink.hidden = true;
        }
      }

      const applyForm = document.querySelector('#candidate-apply-form');
      const applyButton = document.querySelector('#candidate-apply-button');
      const applySuccess = document.querySelector('#apply-success');
      const applySuccessText = applySuccess ? applySuccess.querySelector('span + span') : null;
      const setAppliedState = isApplied => {
        if (!applyButton) return;
        applyButton.textContent = isApplied ? '🗑 Remove Application' : '🚀 Apply Now';
        applyButton.className = isApplied ? 'btn btn-secondary btn-lg' : 'btn btn-primary btn-lg';
        if (applySuccess) {
          applySuccess.hidden = !isApplied;
        }
        if (applySuccessText) {
          applySuccessText.textContent = isApplied
            ? ' You have applied for this job.'
            : ' Your application has been submitted!';
        }
      };

      setAppliedState(Boolean(job.isApplied));

      if (applyForm && applyButton && !applyForm.dataset.applyBound) {
        applyForm.dataset.applyBound = 'true';
        applyForm.addEventListener('submit', async event => {
          event.preventDefault();

          const shouldRemove = applyButton.textContent.includes('Remove');

          try {
            const response = await fetch('/api/candidate/apply', {
              method: 'POST',
              headers: {
                'Content-Type': 'application/json'
              },
              credentials: 'same-origin',
              body: JSON.stringify({ jobId: Number(jobId), removeApplication: shouldRemove })
            });

            const data = await response.json().catch(() => ({}));
            if (!response.ok) {
              return;
            }

            const isApplied = Boolean(data.isApplied);
            setAppliedState(isApplied);

            if (applySuccessText) {
              applySuccessText.textContent = isApplied
                ? ' You have applied for this job.'
                : ' Your application has been removed.';
            } else if (applySuccess) {
              applySuccess.textContent = isApplied
                ? 'You have applied for this job.'
                : 'Your application has been removed.';
            }

            if (applySuccess) {
              applySuccess.hidden = !isApplied;
            }
          } catch {
          }
        });
      }
    } catch {
    }
  }

  async function initializeMembershipPage(options) {
    const primaryAction = options.primaryActionSelector ? document.querySelector(options.primaryActionSelector) : null;
    const premiumButton = options.premiumButtonSelector ? document.querySelector(options.premiumButtonSelector) : null;
    const freeMarker = options.freeMarkerSelector ? document.querySelector(options.freeMarkerSelector) : null;
    const pageBadge = options.pageBadgeSelector ? document.querySelector(options.pageBadgeSelector) : null;

    if (!primaryAction && !premiumButton && !freeMarker && !pageBadge) return;

    const setActionState = (button, disabled) => {
      if (button) button.disabled = disabled;
    };

    const renderStatus = membershipStatus => {
      const normalizedStatus = formatMembershipStatus(membershipStatus);
      applyPlanPresentation({
        status: normalizedStatus,
        subject: options.subject,
        badgeSelector: options.pageBadgeSelector,
        titleSelector: options.titleSelector,
        copySelector: options.copySelector,
        primaryActionSelector: options.primaryActionSelector,
        freeMarkerSelector: options.freeMarkerSelector,
        premiumButtonSelector: options.premiumButtonSelector
      });
    };

    let membershipStatus = 'free';

    try {
      const response = await fetch(options.loadEndpoint, { credentials: 'same-origin' });
      const data = await response.json().catch(() => ({}));
      if (response.ok) {
        membershipStatus = data.membershipStatus || data.membership_status || 'free';
      }
    } catch {
    }

    renderStatus(membershipStatus);

    const bindMembershipButton = button => {
      if (!button || button.dataset.membershipBound === 'true') return;

      button.dataset.membershipBound = 'true';
      button.addEventListener('click', async () => {
        if (button.disabled) return;

        const targetStatus = button.dataset.membershipTarget || 'free';
        const buttons = [primaryAction, premiumButton, freeMarker].filter(Boolean);
        buttons.forEach(item => setActionState(item, true));

        try {
          const data = await updateMembershipStatus(options.updateEndpoint, targetStatus);
          membershipStatus = data.membershipStatus || data.membership_status || targetStatus;
          renderStatus(membershipStatus);
        } catch {
        } finally {
          buttons.forEach(item => {
            const target = item.dataset.membershipTarget || 'free';
            const isCurrentPlan = formatMembershipStatus(membershipStatus).toLowerCase() === formatMembershipStatus(target).toLowerCase();
            setActionState(item, isCurrentPlan);
          });
        }
      });
    };

    [primaryAction, premiumButton, freeMarker].forEach(bindMembershipButton);
  }

  async function loadCandidateMembershipPage() {
    await initializeMembershipPage({
      loadEndpoint: '/api/candidate/dashboard',
      updateEndpoint: '/api/candidate/membership',
      subject: 'job',
      pageBadgeSelector: '#candidate-membership-page-badge',
      titleSelector: '#candidate-current-plan-title',
      copySelector: '#candidate-current-plan-copy',
      primaryActionSelector: '#candidate-membership-primary-action',
      freeMarkerSelector: '#candidate-free-plan-marker',
      premiumButtonSelector: '#candidate-premium-upgrade-button'
    });
  }

  async function loadEmployerMembershipPage() {
    await initializeMembershipPage({
      loadEndpoint: '/api/employer/dashboard',
      updateEndpoint: '/api/employer/membership',
      subject: 'candidate',
      pageBadgeSelector: '#employer-membership-page-badge',
      titleSelector: '#employer-current-plan-title',
      copySelector: '#employer-current-plan-copy',
      primaryActionSelector: '#employer-membership-primary-action',
      freeMarkerSelector: '#employer-free-plan-marker',
      premiumButtonSelector: '#employer-premium-upgrade-button'
    });
  }

  function renderEmployerRecommendationCard(recommendation, options = {}) {
    const matchScore = clampMatchScore(recommendation.matchScore);
    const showMatchedJobs = options.showMatchedJobs !== false;
    const showMatchVisuals = options.showMatchVisuals !== false;
    const candidateId = recommendation.candidateId ?? recommendation.id;
    const matchedJobs = Array.isArray(recommendation.matchedJobs) ? recommendation.matchedJobs : [];
    const matchedJobsMarkup = showMatchedJobs && matchedJobs.length
      ? `
        <div class="candidate-meta-row">
          <span class="candidate-meta-label">Matched jobs:</span>
          <div class="match-reasons-list">
            ${matchedJobs
              .map(job => `<div>${escapeHtml(job.title)} - ${clampMatchScore(job.matchScore)}%</div>`)
              .join('')}
          </div>
        </div>
      `
      : (showMatchedJobs && recommendation.matchedJobTitle
          ? `<div style="font-size:0.85rem; color:var(--text-muted);">Best match job: ${escapeHtml(recommendation.matchedJobTitle)}</div>`
          : '');

    return `
      <a class="card" href="${employerCandidateDetailsUrl(candidateId)}" style="display:block; text-decoration:none; color:inherit;">
        <div style="display:flex; justify-content:space-between; gap:12px; align-items:flex-start;">
          <div>
            <div class="card-title">${escapeHtml(recommendation.fullName)}</div>
            <div style="color:var(--text-muted); margin-bottom:8px;">${escapeHtml(recommendation.major || 'Field not listed')}</div>
          </div>
          ${showMatchVisuals ? `
          <div style="text-align:right; min-width:90px;">
            <div class="score-circle score-circle-sm" style="--score-color:${getMatchColor(matchScore)};">${matchScore}%</div>
            <div style="font-size:0.8rem; color:var(--text-muted);">Match</div>
          </div>
          ` : ''}
        </div>
        <div style="font-size:0.9rem; margin-bottom:8px;">${escapeHtml(recommendation.skills || 'No skills listed')}</div>
        <div style="font-size:0.85rem; color:var(--text-muted); margin-bottom:8px;">${escapeHtml(recommendation.preferredLocation || 'Location not listed')} • ${escapeHtml(recommendation.preferredWorkMode || 'Mode not listed')} • ${escapeHtml(recommendation.experienceText || 'Experience not listed')}</div>
        ${showMatchVisuals ? `
        <div class="match-bar-wrap" style="margin-bottom:10px;">
          <div class="match-bar" style="width:${matchScore}%; ${getMatchBarStyle(matchScore)}"></div>
        </div>
        ` : ''}
        ${matchedJobsMarkup}
      </a>
    `;
  }

  if (salaryMinInput && salaryMaxInput) {
    let autoSyncedSalaryMax = salaryMaxInput.value.trim() !== '' && salaryMaxInput.value === salaryMinInput.value;

    salaryMinInput.addEventListener('input', () => {
      const min = parseFloat(salaryMinInput.value);
      const max = parseFloat(salaryMaxInput.value);

      if (!salaryMaxInput.value.trim() || autoSyncedSalaryMax || (!Number.isNaN(min) && !Number.isNaN(max) && min > max)) {
        salaryMaxInput.value = salaryMinInput.value;
        autoSyncedSalaryMax = true;
      }

      validateSalaryRange();
    });

    salaryMaxInput.addEventListener('input', () => {
      const min = parseFloat(salaryMinInput.value);
      const max = parseFloat(salaryMaxInput.value);

      if (!Number.isNaN(min) && !Number.isNaN(max) && max < min) {
        salaryMinInput.value = salaryMaxInput.value;
      }

      autoSyncedSalaryMax = salaryMaxInput.value.trim() !== '' && salaryMaxInput.value === salaryMinInput.value;
      validateSalaryRange();
    });
  } else {
    if (salaryMinInput) salaryMinInput.addEventListener('input', validateSalaryRange);
    if (salaryMaxInput) salaryMaxInput.addEventListener('input', validateSalaryRange);
  }

  // Validate on form submit
  document.querySelectorAll('form').forEach(form => {
    form.addEventListener('submit', e => {
      if (!validateSalaryRange()) {
        e.preventDefault();
      }
    });
  });

  // ============================================================
  // SKILLS TAG PREVIEW
  // Splits comma-separated skills input and shows pill tags below
  // ============================================================
  function initSkillsPreview(inputSelector, previewSelector) {
    const input   = document.querySelector(inputSelector);
    const preview = document.querySelector(previewSelector);
    if (!input || !preview) return;

    function renderSkillPills(value) {
      preview.innerHTML = '';
      const skills = value.split(',').map(s => s.trim()).filter(s => s.length > 0);
      skills.forEach(skill => {
        const span = document.createElement('span');
        span.className   = 'skill-tag';
        span.textContent = skill;
        preview.appendChild(span);
      });
    }

    input.addEventListener('input', () => renderSkillPills(input.value));
    // Render on page load if value exists
    if (input.value) renderSkillPills(input.value);
  }

  initSkillsPreview('input[name="skills"]',           '#skills-preview');
  initSkillsPreview('input[name="required_skills"]',  '#required-skills-preview');

  // ============================================================
  // AUTO-DISMISS ALERTS
  // ============================================================
  document.querySelectorAll('.alert').forEach(alert => {
    if (alert.hidden) return;

    setTimeout(() => {
      alert.style.transition = 'opacity 0.5s ease, transform 0.5s ease';
      alert.style.opacity    = '0';
      alert.style.transform  = 'translateY(-8px)';
      setTimeout(() => alert.remove(), 500);
    }, 4000);
  });

  // ============================================================
  // SMOOTH SCROLL (for anchor links)
  // ============================================================
  document.querySelectorAll('a[href^="#"]').forEach(anchor => {
    anchor.addEventListener('click', e => {
      const target = document.querySelector(anchor.getAttribute('href'));
      if (target) {
        e.preventDefault();
        target.scrollIntoView({ behavior: 'smooth', block: 'start' });
      }
    });
  });

  // ============================================================
  // FILTER PANEL TOGGLE (mobile)
  // ============================================================
  const filterToggleBtn = document.querySelector('.filter-toggle-btn');
  const filterPanel     = document.querySelector('.filter-panel');

  if (filterToggleBtn && filterPanel) {
    filterToggleBtn.addEventListener('click', () => {
      const isOpen = filterPanel.style.display !== 'none';
      filterPanel.style.display = isOpen ? 'none' : 'block';
      filterToggleBtn.textContent = isOpen ? '⚙ Show Filters' : '✕ Hide Filters';
    });

    // On mobile, collapse by default
    if (window.innerWidth <= 768) {
      filterPanel.style.display = 'none';
      if (filterToggleBtn) filterToggleBtn.textContent = '⚙ Show Filters';
    }
  }

  // ============================================================
  // ANIMATE MATCH BARS ON SCROLL
  // ============================================================
  function animateMatchBars() {
    document.querySelectorAll('.match-bar[data-width]').forEach(bar => {
      const rect = bar.getBoundingClientRect();
      if (rect.top < window.innerHeight) {
        bar.style.width = bar.getAttribute('data-width') + '%';
        bar.removeAttribute('data-width');
      }
    });
  }

  // Set initial width to 0 for animation
  document.querySelectorAll('.match-bar').forEach(bar => {
    const currentWidth = bar.style.width || '0%';
    bar.setAttribute('data-width', parseFloat(currentWidth));
    bar.style.width = '0%';
  });

  window.addEventListener('scroll', animateMatchBars);
  setTimeout(animateMatchBars, 100);

  // ============================================================
  // PASSWORD MATCH VALIDATION
  // ============================================================
  const passwordInput  = document.querySelector('input[name="password"]');
  const confirmInput   = document.querySelector('input[name="confirm_password"]');

  if (passwordInput && confirmInput) {
    function checkPasswordMatch() {
      if (confirmInput.value && passwordInput.value !== confirmInput.value) {
        confirmInput.setCustomValidity('Passwords do not match.');
      } else {
        confirmInput.setCustomValidity('');
      }
    }
    passwordInput.addEventListener('input', checkPasswordMatch);
    confirmInput.addEventListener('input', checkPasswordMatch);
  }

  // ============================================================
  // DEADLINE DATE MIN (prevent past dates on job postings)
  // ============================================================
  const deadlineInput = document.querySelector('input[name="application_deadline"]');
  if (deadlineInput) {
    const today = new Date().toISOString().split('T')[0];
    deadlineInput.setAttribute('min', today);
  }

  // ============================================================
  // COPY TO CLIPBOARD (for URLs)
  // ============================================================
  document.querySelectorAll('[data-copy]').forEach(btn => {
    btn.addEventListener('click', () => {
      const text = btn.getAttribute('data-copy');
      navigator.clipboard.writeText(text).then(() => {
        const original = btn.textContent;
        btn.textContent = 'Copied!';
        setTimeout(() => { btn.textContent = original; }, 2000);
      });
    });
  });

  // ============================================================
  // FORM VALIDATION FEEDBACK – highlight empty required fields
  // ============================================================
  document.querySelectorAll('form').forEach(form => {
    form.addEventListener('submit', () => {
      form.querySelectorAll('[required]').forEach(field => {
        if (!field.value.trim()) {
          field.classList.add('input-error');
          field.addEventListener('input', () => field.classList.remove('input-error'), { once: true });
        }
      });
    });
  });

  // ============================================================
  //C++ Server-side form handling with JSON API
  // ============================================================

    // ============================================================
    // LOGIN FORM -> JSON API
    // ============================================================
    const loginForm = document.querySelector('#login-form');

    if (loginForm) {
        loginForm.addEventListener('submit', async e => {
            e.preventDefault();

            const errorBox = document.querySelector('#error-messages');
            const flashBox = document.querySelector('#flash-message');
            const submitButton = loginForm.querySelector('button[type="submit"]');

            if (errorBox) {
                errorBox.hidden = true;
                errorBox.textContent = '';
            }

            if (flashBox) {
                flashBox.hidden = true;
                flashBox.textContent = '';
            }

            const email = loginForm.querySelector('#email')?.value.trim() || '';
            const password = loginForm.querySelector('#password')?.value || '';

            if (!email || !password) {
                if (errorBox) {
                    errorBox.textContent = 'Email and password are required.';
                    errorBox.hidden = false;
                }
                return;
            }

            try {
                if (submitButton) submitButton.disabled = true;

                const response = await fetch('/api/login', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json'
                    },
                    credentials: 'same-origin',
                    body: JSON.stringify({
                        username: email,
                        password: password
                    })
                });

                const data = await response.json().catch(() => ({}));

                if (!response.ok) {
                    if (errorBox) {
                        errorBox.textContent = response.status === 401
                            ? 'Incorrect email or password.'
                            : (data.error || 'Login failed.');
                        errorBox.hidden = false;
                    }
                    return;
                }

                if (flashBox) {
                    flashBox.textContent = 'Login successful.';
                    flashBox.hidden = false;
                }

                window.location.href = data.redirectTo || '/';
            } catch {
                if (errorBox) {
                    errorBox.textContent = 'Unable to reach the server.';
                    errorBox.hidden = false;
                }
            } finally {
                if (submitButton) submitButton.disabled = false;
            }
        });
    }

    // ============================================================
    // REGISTER FORM -> JSON API
    // ============================================================
    const registerForm = document.querySelector('#register-form');

    if (registerForm) {
        registerForm.addEventListener('submit', async e => {
            e.preventDefault();

            const errorBox = document.querySelector('#error-messages');
            const submitButton = registerForm.querySelector('button[type="submit"]');

            if (errorBox) {
                errorBox.hidden = true;
                errorBox.textContent = '';
            }

            const fullName = registerForm.querySelector('#full_name')?.value.trim() || '';
            const email = registerForm.querySelector('#email')?.value.trim() || '';
            const password = registerForm.querySelector('#password')?.value || '';
            const confirmPassword = registerForm.querySelector('#confirm_password')?.value || '';
            const role = registerForm.querySelector('input[name="role"]:checked')?.value || 'candidate';

            if (!fullName || !email || !password || !confirmPassword) {
                if (errorBox) {
                    errorBox.textContent = 'All fields are required.';
                    errorBox.hidden = false;
                }
                return;
            }

            if (password !== confirmPassword) {
                if (errorBox) {
                    errorBox.textContent = 'Passwords do not match.';
                    errorBox.hidden = false;
                }
                return;
            }

            try {
                if (submitButton) submitButton.disabled = true;

                const response = await fetch('/api/register', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json'
                    },
                    credentials: 'same-origin',
                    body: JSON.stringify({
                        full_name: fullName,
                        email: email,
                        password: password,
                        role: role
                    })
                });

                const data = await response.json().catch(() => ({}));

                if (!response.ok) {
                    if (errorBox) {
                        errorBox.textContent = data.error || 'Registration failed.';
                        errorBox.hidden = false;
                    }
                    return;
                }

                window.location.href = '/login';
            } catch {
                if (errorBox) {
                    errorBox.textContent = 'Unable to reach the server.';
                    errorBox.hidden = false;
                }
            } finally {
                if (submitButton) submitButton.disabled = false;
            }
        });
    }

    // ============================================================
    // CANDIDATE PROFILE FORM -> JSON API
    // ============================================================
    const candidateProfileForm = document.querySelector('#candidate-profile-form');

    async function loadCandidateProfile() {
        if (!candidateProfileForm) return;

        try {
            const response = await fetch('/api/candidate/profile', {
                method: 'GET',
                credentials: 'same-origin'
            });

            const data = await response.json().catch(() => ({}));
            if (!response.ok || !data.profile) return;

            const profile = data.profile;
            Object.entries(profile).forEach(([key, value]) => {
                const field = candidateProfileForm.querySelector(`[name="${key}"]`);
                if (field && value != null) {
                    field.value = value;
                }
            });

            const submitButton = candidateProfileForm.querySelector('button[type="submit"]');
            if (submitButton) {
                submitButton.textContent = '💾 Save Profile';
            }

            const skillsInput = candidateProfileForm.querySelector('[name="skills"]');
            const skillsPreview = document.querySelector('#skills-preview');
            if (skillsInput && skillsPreview) {
                skillsPreview.innerHTML = '';
                skillsInput.value.split(',').map(s => s.trim()).filter(Boolean).forEach(skill => {
                    const span = document.createElement('span');
                    span.className = 'skill-tag';
                    span.textContent = skill;
                    skillsPreview.appendChild(span);
                });
            }
        } catch {
        }
    }

    if (candidateProfileForm) {
        loadCandidateProfile();

        candidateProfileForm.addEventListener('submit', async e => {
            e.preventDefault();

            const successBox = document.querySelector('#success-message');
            const errorBox = document.querySelector('#error-message');
            const submitButton = candidateProfileForm.querySelector('button[type="submit"]');

            if (successBox) {
                successBox.hidden = true;
                successBox.textContent = '';
            }

            if (errorBox) {
                errorBox.hidden = true;
                errorBox.textContent = '';
            }

            const payload = {
                contact_info: candidateProfileForm.querySelector('[name="contact_info"]')?.value.trim() || '',
                education: candidateProfileForm.querySelector('[name="education"]')?.value || '',
                major: candidateProfileForm.querySelector('[name="major"]')?.value.trim() || '',
                years_experience: parseInt(candidateProfileForm.querySelector('[name="years_experience"]')?.value || '0', 10),
                work_experience: candidateProfileForm.querySelector('[name="work_experience"]')?.value.trim() || '',
                skills: candidateProfileForm.querySelector('[name="skills"]')?.value.trim() || '',
                preferred_work_mode: candidateProfileForm.querySelector('[name="preferred_work_mode"]')?.value || '',
                preferred_location: candidateProfileForm.querySelector('[name="preferred_location"]')?.value.trim() || '',
                expected_salary: candidateProfileForm.querySelector('[name="expected_salary"]')?.value || null,
                employment_type: candidateProfileForm.querySelector('[name="employment_type"]')?.value || '',
                summary: candidateProfileForm.querySelector('[name="summary"]')?.value.trim() || '',
                portfolio_url: candidateProfileForm.querySelector('[name="portfolio_url"]')?.value.trim() || ''
            };

            try {
                if (submitButton) submitButton.disabled = true;

                const response = await fetch('/api/candidate/profile', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json'
                    },
                    credentials: 'same-origin',
                    body: JSON.stringify(payload)
                });

                const data = await response.json().catch(() => ({}));

                if (!response.ok) {
                    if (errorBox) {
                        errorBox.textContent = data.error || 'Failed to save profile.';
                        errorBox.hidden = false;
                    }
                    return;
                }

                if (successBox) {
                    successBox.textContent = 'Profile saved successfully.';
                    successBox.hidden = false;
                }

                window.location.href = '/candidate/dashboard';
            } catch {
                if (errorBox) {
                    errorBox.textContent = 'Unable to reach the server.';
                    errorBox.hidden = false;
                }
            } finally {
                if (submitButton) submitButton.disabled = false;
            }
        });
    }

    // ============================================================
    // CREATE JOB FORM -> JSON API
    // ============================================================
    const createJobForm = document.querySelector('#create-job-form');

    if (createJobForm) {
        createJobForm.addEventListener('submit', async e => {
            e.preventDefault();

            const errorBox = document.querySelector('#error-message');
            const submitButton = createJobForm.querySelector('button[type="submit"]');

            if (errorBox) {
                errorBox.hidden = true;
                errorBox.textContent = '';
            }

            const payload = {
                job_title: createJobForm.querySelector('[name="job_title"]')?.value.trim() || '',
                job_description: createJobForm.querySelector('[name="job_description"]')?.value.trim() || '',
                required_skills: createJobForm.querySelector('[name="required_skills"]')?.value.trim() || '',
                required_education: createJobForm.querySelector('[name="required_education"]')?.value || 'Any',
                required_experience: parseInt(createJobForm.querySelector('[name="required_experience"]')?.value || '0', 10),
                career_level: createJobForm.querySelector('[name="career_level"]')?.value || 'Entry-level',
                job_type: createJobForm.querySelector('[name="job_type"]')?.value || 'Full-time',
                work_mode: createJobForm.querySelector('[name="work_mode"]')?.value || 'On-site',
                job_location: createJobForm.querySelector('[name="job_location"]')?.value.trim() || '',
                salary_min: createJobForm.querySelector('[name="salary_min"]')?.value || null,
                salary_max: createJobForm.querySelector('[name="salary_max"]')?.value || null,
                application_deadline: createJobForm.querySelector('[name="application_deadline"]')?.value || null,
                status: createJobForm.querySelector('[name="status"]')?.value || 'Open'
            };

            try {
                if (submitButton) submitButton.disabled = true;

                const response = await fetch('/api/employer/jobs', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json'
                    },
                    credentials: 'same-origin',
                    body: JSON.stringify(payload)
                });

                const data = await response.json().catch(() => ({}));

                if (!response.ok) {
                    if (errorBox) {
                        errorBox.textContent = data.error || 'Failed to create job.';
                        errorBox.hidden = false;
                    }
                    return;
                }

                window.location.href = '/employer/manage-jobs';
            } catch {
                if (errorBox) {
                    errorBox.textContent = 'Unable to reach the server.';
                    errorBox.hidden = false;
                }
            } finally {
                if (submitButton) submitButton.disabled = false;
            }
        });
    }

    // ============================================================
    // ADMIN DASHBOARD
    // ============================================================
    async function loadAdminDashboard() {
        const recentUsersTable = document.querySelector('#recentUsersTable');
        const recentJobsTable = document.querySelector('#recentJobsTable');
        if (!recentUsersTable && !recentJobsTable) return;

        try {
            const response = await fetch('/api/admin/dashboard', { credentials: 'same-origin' });
            const data = await response.json().catch(() => ({}));
            if (!response.ok) return;

            const setText = (selector, value) => {
                const el = document.querySelector(selector);
                if (el) el.textContent = String(value ?? 0);
            };

            setText('#totalUsers', data.totalUsers);
            setText('#totalCandidates', data.totalCandidates);
            setText('#totalEmployers', data.totalEmployers);
            setText('#premiumUsers', data.premiumUsers);
            setText('#totalJobs', data.totalJobs);
            setText('#openJobs', data.openJobs);
            setText('#totalApplications', data.totalApplications);
            setText('#totalProfiles', data.totalProfiles);
            setText('#totalCompanies', data.totalCompanies);

            if (recentUsersTable) {
                const users = data.recentUsers || [];
                recentUsersTable.innerHTML = users.length
                    ? users.map(user => `
                <tr>
                  <td>${getAdminUserDetailsUrl(user)
                        ? `<a href="${getAdminUserDetailsUrl(user)}" style="color:inherit; text-decoration:none; font-weight:600;">${escapeHtml(user.full_name ?? '')}</a>`
                        : escapeHtml(user.full_name ?? '')}</td>
                  <td>${escapeHtml(user.role ?? '')}</td>
                  <td>${escapeHtml(user.membership_status ?? '')}</td>
                  <td>${escapeHtml(user.created_at ?? '')}</td>
                </tr>
              `).join('')
                    : '<tr><td colspan="4" style="text-align:center; color:var(--text-muted); padding:24px;">No recent users.</td></tr>';
            }

            if (recentJobsTable) {
                const jobs = data.recentJobs || [];
                recentJobsTable.innerHTML = jobs.length
                    ? jobs.map(job => `
                <tr>
                  <td><a href="${adminJobDetailsUrl(job.id)}" style="color:inherit; text-decoration:none; font-weight:600;">${escapeHtml(job.title ?? '')}</a></td>
                  <td>${escapeHtml(job.company ?? '')}</td>
                  <td>${escapeHtml(job.status ?? '')}</td>
                  <td>${escapeHtml(job.posted ?? '')}</td>
                </tr>
              `).join('')
                    : '<tr><td colspan="4" style="text-align:center; color:var(--text-muted); padding:24px;">No recent jobs.</td></tr>';
            }
        } catch {
            if (recentUsersTable) {
                recentUsersTable.innerHTML = '<tr><td colspan="4" style="text-align:center; color:var(--error); padding:24px;">Unable to load recent users.</td></tr>';
            }
            if (recentJobsTable) {
                recentJobsTable.innerHTML = '<tr><td colspan="4" style="text-align:center; color:var(--error); padding:24px;">Unable to load recent jobs.</td></tr>';
            }
        }
    }

    // ============================================================
    // ADMIN USERS LIST -> JSON API
    // ============================================================
    const adminUsersFilterForm = document.querySelector('#admin-users-filter-form');
    const usersTableBody = document.querySelector('#users-table-body');
    const totalUsers = document.querySelector('#totalUsers');

    async function loadAdminUsers() {
        if (!usersTableBody) return;

        const urlParams = new URLSearchParams(window.location.search);
        const searchField = adminUsersFilterForm?.querySelector('[name="search"]');
        const roleField = adminUsersFilterForm?.querySelector('[name="role"]');
        if (searchField && urlParams.has('search')) searchField.value = urlParams.get('search') || '';
        if (roleField && urlParams.has('role')) roleField.value = urlParams.get('role') || '';

        const search = adminUsersFilterForm?.querySelector('[name="search"]')?.value.trim() || '';
        const role = adminUsersFilterForm?.querySelector('[name="role"]')?.value || '';

        const params = new URLSearchParams();
        if (search) params.set('search', search);
        if (role) params.set('role', role);

        try {
            const response = await fetch(`/api/admin/users?${params.toString()}`, {
                method: 'GET',
                credentials: 'same-origin'
            });

            const data = await response.json().catch(() => ({}));

            if (!response.ok) {
                usersTableBody.innerHTML = '<tr><td colspan="8" style="text-align:center; color:var(--error); padding:40px;">Failed to load users.</td></tr>';
                return;
            }

            const users = data.users || [];
            if (totalUsers) totalUsers.textContent = String(users.length);

            if (users.length === 0) {
                usersTableBody.innerHTML = '<tr><td colspan="8" style="text-align:center; color:var(--text-muted); padding:40px;">No users found.</td></tr>';
                return;
            }

            usersTableBody.innerHTML = users.map((user, index) => `
        <tr>
          <td>${index + 1}</td>
          <td>${escapeHtml(user.full_name ?? '')}</td>
          <td>${escapeHtml(user.email ?? '')}</td>
          <td>${escapeHtml(user.role ?? '')}</td>
          <td>${escapeHtml(user.membership_status ?? '')}</td>
          <td>${escapeHtml(user.profile_or_jobs ?? '-')}</td>
          <td>${escapeHtml(user.created_at ?? '')}</td>
          <td>${getAdminUserDetailsUrl(user)
            ? `<a href="${getAdminUserDetailsUrl(user)}" class="btn btn-secondary btn-sm">View</a>`
            : '<span style="color:var(--text-muted); font-size:0.85rem;">N/A</span>'}</td>
        </tr>
      `).join('');
        } catch {
            usersTableBody.innerHTML = '<tr><td colspan="8" style="text-align:center; color:var(--error); padding:40px;">Unable to reach the server.</td></tr>';
        }
    }

    if (adminUsersFilterForm) {
        adminUsersFilterForm.addEventListener('submit', e => {
            e.preventDefault();
            loadAdminUsers();
        });

        loadAdminUsers();
    }

    // ============================================================
    // ADMIN JOBS
    // ============================================================
    const adminJobsForm = document.querySelector('form[action="/admin/jobs"]');
    const adminJobsBody = document.querySelector('#jobs-table-body');

    async function loadAdminJobs() {
        if (!adminJobsForm || !adminJobsBody) return;

        const searchField = adminJobsForm.querySelector('[name="search"]');
        const statusField = adminJobsForm.querySelector('[name="status"]');
        const urlParams = new URLSearchParams(window.location.search);
        if (searchField && urlParams.has('search')) searchField.value = urlParams.get('search') || '';
        if (statusField && urlParams.has('status')) statusField.value = urlParams.get('status') || '';

        const params = new URLSearchParams(new FormData(adminJobsForm));
        try {
            const response = await fetch(`/api/admin/jobs?${params.toString()}`, { credentials: 'same-origin' });
            const data = await response.json().catch(() => ({}));
            if (!response.ok) {
                adminJobsBody.innerHTML = '<tr><td colspan="9" style="text-align:center; color:var(--error); padding:40px;">Failed to load jobs.</td></tr>';
                return;
            }

            const jobs = data.jobs || [];
            const totalJobs = document.querySelector('#totalJobs');
            if (totalJobs) totalJobs.textContent = String(jobs.length);

            adminJobsBody.innerHTML = jobs.length
                ? jobs.map((job, index) => `
            <tr>
              <td>${index + 1}</td>
              <td>${escapeHtml(job.title)}</td>
              <td>${escapeHtml(job.company)}</td>
              <td>${escapeHtml(job.type)}</td>
              <td>${escapeHtml(job.status)}</td>
              <td>${escapeHtml(String(job.applicationCount ?? 0))}</td>
              <td>${escapeHtml(job.deadline)}</td>
              <td>${escapeHtml(job.posted)}</td>
              <td><a href="${adminJobDetailsUrl(job.id)}" class="btn btn-secondary btn-sm">View</a></td>
            </tr>
          `).join('')
                : '<tr><td colspan="9" style="text-align:center; color:var(--text-muted); padding:40px;">No jobs found.</td></tr>';
        } catch {
            adminJobsBody.innerHTML = '<tr><td colspan="9" style="text-align:center; color:var(--error); padding:40px;">Unable to reach the server.</td></tr>';
        }
    }

    if (adminJobsForm) {
        adminJobsForm.addEventListener('submit', e => {
            e.preventDefault();
            loadAdminJobs();
        });

        loadAdminJobs();
    }

    // ============================================================
    // ADMIN CANDIDATE DETAILS
    // ============================================================

    async function loadAdminCandidateDetails() {
        const name = document.querySelector('#admin-candidate-name');
        if (!name) return;

        const params = new URLSearchParams(window.location.search);
        const candidateId = params.get('id');
        if (!candidateId) return;

        try {
            const response = await fetch(`/api/admin/candidate?id=${encodeURIComponent(candidateId)}`, { credentials: 'same-origin' });
            const data = await response.json().catch(() => ({}));
            if (!response.ok || !data.candidate) return;

            const candidate = data.candidate;
            const setText = (selector, value, fallback = '') => {
                const el = document.querySelector(selector);
                if (el) el.textContent = value || fallback;
            };

            setText('#admin-candidate-name', candidate.fullName, 'Candidate');
            setText('#admin-candidate-summary-meta', `${candidate.education || 'Education not listed'} · ${candidate.experienceText || 'Experience not listed'}`);
            setText('#admin-candidate-about-text', candidate.summary, 'No summary provided.');
            setText('#admin-candidate-work-experience', candidate.workExperience, 'No work experience listed.');
            setText('#admin-candidate-education', candidate.education, 'Not provided');
            setText('#admin-candidate-major', candidate.major, 'Not provided');
            setText('#admin-candidate-experience', candidate.experienceText, 'Not provided');
            setText('#admin-candidate-employment-type', candidate.employmentType, 'Not provided');
            setText('#admin-candidate-work-mode', candidate.preferredWorkMode, 'Not provided');
            setText('#admin-candidate-preferred-location', candidate.preferredLocation, 'Not provided');
            setText('#admin-candidate-expected-salary', candidate.expectedSalary, 'Not provided');
            setText('#admin-candidate-contact-info', candidate.contactInfo, 'Not provided');
            setText('#admin-candidate-member-since', candidate.createdAt, 'Not provided');
            setText('#admin-candidate-work-mode-badge', candidate.preferredWorkMode, 'Not provided');

            const skillsContainer = document.querySelector('#admin-candidate-skills-container');
            if (skillsContainer) {
                skillsContainer.innerHTML = renderSkillBadges(candidate.skills) || '<span style="color:var(--text-muted);">No skills listed.</span>';
            }

            const portfolioLink = document.querySelector('#admin-candidate-portfolio-link');
            if (portfolioLink) {
                if (candidate.portfolioUrl) {
                    portfolioLink.href = candidate.portfolioUrl;
                    portfolioLink.hidden = false;
                } else {
                    portfolioLink.hidden = true;
                }
            }
        } catch {
        }
    }

    // ============================================================
    // ADMIN EMPLOYER DETAILS
    // ============================================================

    async function loadAdminEmployerDetails() {
        const companyName = document.querySelector('#admin-employer-company-name');
        if (!companyName) return;

        const params = new URLSearchParams(window.location.search);
        const employerId = params.get('id');
        if (!employerId) return;

        try {
            const response = await fetch(`/api/admin/employer?id=${encodeURIComponent(employerId)}`, { credentials: 'same-origin' });
            const data = await response.json().catch(() => ({}));
            if (!response.ok || !data.employer) return;

            const employer = data.employer;
            const setText = (selector, value, fallback = '') => {
                const el = document.querySelector(selector);
                if (el) el.textContent = value || fallback;
            };

            setText('#admin-employer-company-name', employer.companyName, employer.fullName || 'Employer');
            setText('#admin-employer-owner-name', employer.fullName, 'Employer');
            setText('#admin-employer-industry', employer.industry, 'Industry not provided');
            setText('#admin-employer-company-size', employer.companySize, 'Company size not provided');
            setText('#admin-employer-total-jobs', `${Number(employer.totalJobs || 0)} jobs posted`);
            setText('#admin-employer-company-description', employer.companyDescription, 'No company description provided.');
            setText('#admin-employer-email', employer.email, 'Not provided');
            setText('#admin-employer-membership', employer.membershipStatus, 'Not provided');
            setText('#admin-employer-company-email', employer.companyEmail, 'Not provided');
            setText('#admin-employer-company-phone', employer.companyPhone, 'Not provided');
            setText('#admin-employer-company-location', employer.companyLocation, 'Not provided');
            setText('#admin-employer-created-at', employer.createdAt, 'Not provided');
            setText('#admin-employer-website-url', employer.websiteUrl, 'Not provided');

            const websiteLink = document.querySelector('#admin-employer-website-link');
            if (websiteLink) {
                if (employer.websiteUrl) {
                    websiteLink.href = employer.websiteUrl;
                    websiteLink.hidden = false;
                } else {
                    websiteLink.hidden = true;
                }
            }
        } catch {
        }
    }

    // ============================================================
    // ADMIN JOB DETAILS
    // ============================================================
    async function loadAdminJobDetails() {
        const title = document.querySelector('#admin-job-title');
        if (!title) return;

        const params = new URLSearchParams(window.location.search);
        const jobId = params.get('id');
        if (!jobId) return;

        try {
            const response = await fetch(`/api/admin/job?id=${encodeURIComponent(jobId)}`, { credentials: 'same-origin' });
            const data = await response.json().catch(() => ({}));
            if (!response.ok || !data.job) return;

            const job = data.job;
            const setText = (selector, value, fallback = '') => {
                const el = document.querySelector(selector);
                if (el) el.textContent = value || fallback;
            };

            setText('#admin-job-title', job.title, 'Job Title');
            setText('#admin-company-name', job.company, 'Company');
            setText('#admin-job-status', job.status, 'Unknown');
            setText('#admin-job-location', `📍 ${job.location || 'Location not listed'}`);
            setText('#admin-job-workmode', job.workMode || 'Mode not listed');
            setText('#admin-job-type', `💼 ${job.type || 'Type not listed'}`);
            setText('#admin-job-level', `📊 ${job.careerLevel || 'Level not listed'}`);
            setText('#admin-job-salary', `💰 ${job.salaryRange || 'Salary not listed'}`);
            setText('#admin-job-description', job.jobDescription, 'No job description available.');
            setText('#admin-company-description', job.companyDescription, 'No company description available.');
            setText('#admin-required-education', job.requiredEducation, 'Not specified');
            setText('#admin-required-experience', job.requiredExperience ? `${job.requiredExperience} years` : 'Not specified');
            setText('#admin-application-count', `${Number(job.applicationCount || 0)} applications`, '0 applications');
            setText('#admin-detail-job-type', job.type, 'Not specified');
            setText('#admin-detail-career-level', job.careerLevel, 'Not specified');
            setText('#admin-application-deadline', job.deadline, 'Not specified');
            setText('#admin-detail-salary-range', job.salaryRange, 'Not specified');

            const skillsContainer = document.querySelector('#admin-required-skills');
            if (skillsContainer) {
                skillsContainer.innerHTML = renderSkillBadges(job.requiredSkills) || '<span style="color:var(--text-muted);">No skills listed.</span>';
            }

            const companyWebsiteLink = document.querySelector('#admin-company-website-link');
            if (companyWebsiteLink) {
                if (job.companyWebsiteUrl) {
                    companyWebsiteLink.href = job.companyWebsiteUrl;
                    companyWebsiteLink.hidden = false;
                } else {
                    companyWebsiteLink.hidden = true;
                }
            }
        } catch {
        }
    }

    runPageInitializer(window.location.pathname, {
        '/candidate/dashboard': typeof loadCandidateDashboard === 'function' ? loadCandidateDashboard : null,
        '/candidate/jobs': typeof loadCandidateJobs === 'function' ? loadCandidateJobs : null,
        '/candidate/applications': typeof loadCandidateApplications === 'function' ? loadCandidateApplications : null,
        '/candidate/membership': typeof loadCandidateMembershipPage === 'function' ? loadCandidateMembershipPage : null,
        '/candidate/job': typeof loadCandidateJobDetails === 'function' ? loadCandidateJobDetails : null,
        '/candidate/recommended-jobs': typeof loadRecommendedJobsPage === 'function' ? loadRecommendedJobsPage : null,
        '/employer/dashboard': typeof loadEmployerDashboard === 'function' ? loadEmployerDashboard : null,
        '/employer/candidates': typeof loadEmployerCandidates === 'function' ? loadEmployerCandidates : null,
        '/employer/membership': typeof loadEmployerMembershipPage === 'function' ? loadEmployerMembershipPage : null,
        '/employer/recommended-candidates': typeof loadEmployerRecommendedCandidatesPage === 'function' ? loadEmployerRecommendedCandidatesPage : null,
        '/employer/recommended_candidates': typeof loadEmployerRecommendedCandidatesPage === 'function' ? loadEmployerRecommendedCandidatesPage : null,
        '/employer/applicants': typeof loadEmployerApplicants === 'function' ? loadEmployerApplicants : null,
        '/employer/view-applicants': typeof loadEmployerApplicants === 'function' ? loadEmployerApplicants : null,
        '/employer/view_applicants': typeof loadEmployerApplicants === 'function' ? loadEmployerApplicants : null,
        '/employer/candidate': typeof loadEmployerCandidateDetails === 'function' ? loadEmployerCandidateDetails : null,
        '/employer/candidate-details': typeof loadEmployerCandidateDetails === 'function' ? loadEmployerCandidateDetails : null,
        '/employer/candidate_details': typeof loadEmployerCandidateDetails === 'function' ? loadEmployerCandidateDetails : null,
        '/employer/manage-jobs': typeof loadEmployerJobs === 'function' ? loadEmployerJobs : null,
        '/employer/manage_jobs': typeof loadEmployerJobs === 'function' ? loadEmployerJobs : null,
        '/employer/jobs': typeof loadEmployerJobs === 'function' ? loadEmployerJobs : null,
        '/employer/edit-job': typeof loadEmployerEditJob === 'function' ? loadEmployerEditJob : null,
        '/employer/edit_job': typeof loadEmployerEditJob === 'function' ? loadEmployerEditJob : null,
        '/admin': typeof loadAdminDashboard === 'function' ? loadAdminDashboard : null,
        '/admin/dashboard': typeof loadAdminDashboard === 'function' ? loadAdminDashboard : null,
        '/admin/users': typeof loadAdminUsers === 'function' ? loadAdminUsers : null,
        '/admin/jobs': typeof loadAdminJobs === 'function' ? loadAdminJobs : null,
        '/admin/candidate': typeof loadAdminCandidateDetails === 'function' ? loadAdminCandidateDetails : null,
        '/admin/employer': typeof loadAdminEmployerDetails === 'function' ? loadAdminEmployerDetails : null,
        '/admin/job': typeof loadAdminJobDetails === 'function' ? loadAdminJobDetails : null
    });

})();

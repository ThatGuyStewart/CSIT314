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
    if (freeMarker) freeMarker.textContent = isPremium ? 'Available Plan' : 'Current Plan';
    if (freeMarker) freeMarker.disabled = !isPremium;
    if (freeMarker) freeMarker.style.cursor = isPremium ? 'pointer' : 'default';
    if (premiumButton) premiumButton.textContent = isPremium ? 'Current Plan' : '⭐ Upgrade Now';
    if (premiumButton) premiumButton.disabled = isPremium;
    if (primaryAction) primaryAction.textContent = isPremium ? 'Current Plan: Premium' : '⭐ Upgrade to Premium';
    if (primaryAction) primaryAction.disabled = isPremium;
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

  function renderEmployerRecommendationCard(recommendation, options = {}) {
    const matchScore = clampMatchScore(recommendation.matchScore);
    const showMatchedJobs = options.showMatchedJobs !== false;
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
          <div style="text-align:right; min-width:90px;">
            <div class="score-circle score-circle-sm" style="--score-color:${getMatchColor(matchScore)};">${matchScore}%</div>
            <div style="font-size:0.8rem; color:var(--text-muted);">Match</div>
          </div>
        </div>
        <div style="font-size:0.9rem; margin-bottom:8px;">${escapeHtml(recommendation.skills || 'No skills listed')}</div>
        <div style="font-size:0.85rem; color:var(--text-muted); margin-bottom:8px;">${escapeHtml(recommendation.preferredLocation || 'Location not listed')} • ${escapeHtml(recommendation.preferredWorkMode || 'Mode not listed')} • ${escapeHtml(recommendation.experienceText || 'Experience not listed')}</div>
        <div class="match-bar-wrap" style="margin-bottom:10px;">
          <div class="match-bar" style="width:${matchScore}%; ${getMatchBarStyle(matchScore)}"></div>
        </div>
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
    // ADMIN USERS LIST -> JSON API
    // ============================================================
    const adminUsersFilterForm = document.querySelector('#admin-users-filter-form');
    const usersTableBody = document.querySelector('#users-table-body');
    const totalUsers = document.querySelector('#totalUsers');

    async function loadAdminUsers() {
        if (!usersTableBody) return;

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
          <td>${user.full_name ?? ''}</td>
          <td>${user.email ?? ''}</td>
          <td>${user.role ?? ''}</td>
          <td>${user.membership_status ?? ''}</td>
          <td>${user.profile_or_jobs ?? '-'}</td>
          <td>${user.created_at ?? ''}</td>
          <td><button type="button" class="btn btn-secondary btn-sm">View</button></td>
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
    // CANDIDATE DASHBOARD
    // ============================================================
    function jobDetailsUrl(jobId) {
        return `/candidate/job?id=${encodeURIComponent(jobId)}`;
    }

    async function loadCandidateDashboard() {
        if (!document.querySelector('#recommendedCount')) return;

        try {
            const response = await fetch('/api/candidate/dashboard', { credentials: 'same-origin' });
            const data = await response.json().catch(() => ({}));
            if (!response.ok) return;

            const recommendedCount = document.querySelector('#recommendedCount');
            const profileComplete = document.querySelector('#profileComplete');
            const jobCount = document.querySelector('#jobCount');
            const topRecommendations = document.querySelector('#top-recommendations');
            const completionBar = document.querySelector('.completion-bar');
            const completionText = document.querySelector('.completion-bar-wrap + div span');
            const membershipStatus = data.membershipStatus || 'free';

            if (recommendedCount) recommendedCount.textContent = data.recommendedCount ?? 0;
            if (profileComplete) profileComplete.textContent = `${data.profileComplete ?? 0}%`;
            if (jobCount) jobCount.textContent = data.jobCount ?? 0;
            if (completionBar) completionBar.style.width = `${data.profileComplete ?? 0}%`;
            if (completionText) completionText.textContent = `${data.profileComplete ?? 0}% complete`;

            applyPlanPresentation({
                status: membershipStatus,
                subject: 'job',
                badgeSelector: '#candidate-plan-badge',
                nameSelector: '#candidate-membership-name',
                limitSelector: '#candidate-rec-limit',
                copySelector: '#candidate-membership-copy',
                badgeSelectorFallback: '#candidate-membership-badge'
            });
            const membershipBadge = document.querySelector('#candidate-membership-badge');
            if (membershipBadge) membershipBadge.textContent = formatMembershipStatus(membershipStatus);

            if (topRecommendations) {
                const jobs = data.topRecommendations || [];
                topRecommendations.innerHTML = jobs.length
                    ? jobs.map(job => renderCandidateRecommendedJobCard(job).replace('recommended-job-card', 'recommended-job-card mb-16')).join('')
                    : '<div class="card">No recommendations available.</div>';
            }
        } catch {
        }
    }

    // ============================================================
    // CANDIDATE JOBS
    // ============================================================
    const candidateJobsForm = document.querySelector('form[action="/candidate/jobs"]');
    const jobsGrid = document.querySelector('#jobs-grid');

    function populateCandidateJobsFiltersFromUrl() {
        if (!candidateJobsForm) return;

        const params = new URLSearchParams(window.location.search);
        candidateJobsForm.querySelectorAll('[name]').forEach(field => {
            const value = params.get(field.name);
            if (value !== null) {
                field.value = value;
            }
        });
    }

    function syncCandidateJobsUrl() {
        if (!candidateJobsForm) return;

        const params = new URLSearchParams();
        new FormData(candidateJobsForm).forEach((value, key) => {
            const text = String(value).trim();
            if (text) {
                params.set(key, text);
            }
        });

        const query = params.toString();
        const targetUrl = query ? `/candidate/jobs?${query}` : '/candidate/jobs';
        window.history.replaceState({}, '', targetUrl);
    }

    async function loadCandidateJobs() {
        if (!jobsGrid || !candidateJobsForm) return;

        const params = new URLSearchParams(new FormData(candidateJobsForm));
        try {
            const response = await fetch(`/api/candidate/jobs?${params.toString()}`, { credentials: 'same-origin' });
            const data = await response.json().catch(() => ({}));
            if (!response.ok) return;

            const jobs = data.jobs || [];
            const shownCount = document.querySelector('#shownCount');
            const totalCount = document.querySelector('#totalCount');

            if (shownCount) shownCount.textContent = String(jobs.length);
            if (totalCount) totalCount.textContent = String(jobs.length);

            jobsGrid.innerHTML = jobs.length
                ? jobs.map(job => `
            <a class="card" href="${jobDetailsUrl(job.id)}" style="display:block; text-decoration:none; color:inherit;">
              <div class="card-title">${job.title}</div>
              <div style="color:var(--text-muted); margin-bottom:8px;">${job.company}</div>
              <div style="font-size:0.9rem; color:var(--text-muted);">${job.location} • ${job.workMode} • ${job.type}</div>
              <div style="margin-top:8px;">${job.salaryRange || 'Salary not listed'}</div>
              <div style="margin-top:8px; font-size:0.85rem; color:var(--text-muted);">Deadline: ${job.deadline || 'N/A'}</div>
            </a>
          `).join('')
                : '<div class="card">No jobs found.</div>';
        } catch {
        }
    }

    if (candidateJobsForm && jobsGrid) {
        populateCandidateJobsFiltersFromUrl();
        candidateJobsForm.addEventListener('submit', e => {
            e.preventDefault();
            syncCandidateJobsUrl();
            loadCandidateJobs();
        });
        loadCandidateJobs();
    }

    // ============================================================
    // CANDIDATE RECOMMENDED JOBS
    // ============================================================
    async function loadRecommendedJobsPage() {
        const container = document.querySelector('#recommendations-grid');
        if (!container) return;

        try {
            const response = await fetch('/api/candidate/recommended-jobs', { credentials: 'same-origin' });
            const data = await response.json().catch(() => ({}));
            if (!response.ok) return;

            const jobs = data.jobs || [];
            const visibleCount = document.querySelector('#visible-count');
            const totalCount = document.querySelector('#total-count');

            if (visibleCount) visibleCount.textContent = String(jobs.length);
            if (totalCount) totalCount.textContent = String(jobs.length);

            container.innerHTML = jobs.length
                ? jobs.map(job => renderCandidateRecommendedJobCard(job)).join('')
                : '<div class="card">No recommendations available.</div>';
        } catch {
        }
    }

    // ============================================================
    // CANDIDATE JOB DETAILS
    // ============================================================
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
            setText('#company-name', job.company ? `🏢 ${job.company}` : '🏢 Company');
            setText('#job-status', job.status, 'Open');
            setText('#job-location', job.location ? `📍 ${job.location}` : '📍 Location not listed');
            setText('#job-workmode', job.workMode || '');
            setText('#job-type', job.type ? `💼 ${job.type}` : '💼 Type not listed');
            setText('#job-level', job.careerLevel ? `📊 ${job.careerLevel}` : '📊 Level not listed');
            setText('#job-salary', job.salaryRange ? `💰 ${job.salaryRange}` : '💰 Salary not listed');
            setText('#job-description', job.jobDescription, 'No job description provided.');
            setText('#company-description', job.companyDescription, 'No company description provided.');
            setText('#required-education', job.requiredEducation, 'Not specified');
            setText('#required-experience', Number.isFinite(job.requiredExperience) ? `${job.requiredExperience} years` : 'Not specified');
            setText('#detail-job-type', job.type, 'Not specified');
            setText('#detail-career-level', job.careerLevel, 'Not specified');
            setText('#application-deadline', job.deadline, 'Not specified');
            setText('#detail-salary-range', job.salaryRange, 'Not specified');

            const matchScore = clampMatchScore(job.matchScore);
            setText('#match-score-circle', `${matchScore}%`, '0%');
            setText('#match-score-text', `${matchScore}%`, '0%');
            applyScoreCircleColor('#match-score-circle', matchScore);

            const matchBar = document.querySelector('#match-score-bar');
            if (matchBar) {
                matchBar.style.width = `${matchScore}%`;
            }
            applyMatchBarColor('#match-score-bar', matchScore);

            const matchReasons = document.querySelector('#match-reasons');
            if (matchReasons) {
                const reasons = Array.isArray(job.matchReasons) ? job.matchReasons : [];
                matchReasons.innerHTML = reasons.length
                    ? reasons.map(reason => `<li>${reason}</li>`).join('')
                    : '<li>No personalized match reasons available.</li>';
            }

            const skillsContainer = document.querySelector('#required-skills');
            if (skillsContainer) {
                const skills = (job.requiredSkills || '').split(',').map(skill => skill.trim()).filter(Boolean);
                skillsContainer.innerHTML = skills.length
                    ? skills.map(skill => `<span class="skill-tag">${skill}</span>`).join('')
                    : '<span style="color:var(--text-muted);">No specific skills listed.</span>';
            }

            const websiteLink = document.querySelector('a[target="_blank"].btn.btn-secondary.btn-sm');
            if (websiteLink) {
                if (job.companyWebsiteUrl) {
                    websiteLink.href = job.companyWebsiteUrl;
                    websiteLink.hidden = false;
                } else {
                    websiteLink.hidden = true;
                }
            }
        } catch {
        }
    }

    // ============================================================
    // EMPLOYER DASHBOARD
    // ============================================================
    async function loadEmployerDashboard() {
        if (!document.querySelector('#activeCount')) return;

        try {
            const dashboardResponse = await fetch('/api/employer/dashboard', { credentials: 'same-origin' });
            const data = await dashboardResponse.json().catch(() => ({}));
            if (!dashboardResponse.ok) return;

            const activeCount = document.querySelector('#activeCount');
            const totalPosts = document.querySelector('#totalPosts');
            const totalCandidates = document.querySelector('#totalCandidates');
            const topCandidates = document.querySelector('#top-candidates');
            const membershipStatus = data.membershipStatus || 'free';

            if (activeCount) activeCount.textContent = data.activeCount ?? 0;
            if (totalPosts) totalPosts.textContent = data.totalPosts ?? 0;
            if (totalCandidates) totalCandidates.textContent = data.totalCandidates ?? 0;

            applyPlanPresentation({
                status: membershipStatus,
                subject: 'candidate',
                badgeSelector: '#employer-plan-badge',
                nameSelector: '#employer-membership-name',
                limitSelector: '#employer-rec-limit',
                copySelector: '#employer-membership-copy'
            });
            const employerMembershipBadge = document.querySelector('#employer-membership-badge');
            if (employerMembershipBadge) employerMembershipBadge.textContent = formatMembershipStatus(membershipStatus);

            if (topCandidates) {
                const candidates = data.topCandidates || [];
                topCandidates.innerHTML = candidates.length
                    ? candidates.map(candidate => renderEmployerRecommendationCard(candidate, { showMatchedJobs: true })).join('')
                    : '<div class="card">No candidates available.</div>';
            }
        } catch {
        }
    }

    async function loadCandidateMembershipPage() {
        if (!document.querySelector('#candidate-membership-page-badge')) return;

        try {
            const response = await fetch('/api/candidate/dashboard', { credentials: 'same-origin' });
            const data = await response.json().catch(() => ({}));
            if (!response.ok) return;

            applyPlanPresentation({
                status: data.membershipStatus || 'free',
                subject: 'job',
                badgeSelector: '#candidate-membership-page-badge',
                titleSelector: '#candidate-current-plan-title',
                copySelector: '#candidate-current-plan-copy',
                freeMarkerSelector: '#candidate-free-plan-marker',
                premiumButtonSelector: '#candidate-premium-upgrade-button',
                primaryActionSelector: '#candidate-membership-primary-action'
            });

            const premiumButton = document.querySelector('#candidate-premium-upgrade-button');
            const primaryAction = document.querySelector('#candidate-membership-primary-action');
            const freeMarker = document.querySelector('#candidate-free-plan-marker');
            const isPremium = (data.membershipStatus || '').toLowerCase() === 'premium';

            if (premiumButton) {
                premiumButton.onclick = async () => {
                    if (premiumButton.disabled) return;
                    await updateMembershipStatus('/api/candidate/membership', 'premium');
                    await loadCandidateMembershipPage();
                };
            }

            if (primaryAction) {
                primaryAction.onclick = async () => {
                    if (primaryAction.disabled) return;
                    await updateMembershipStatus('/api/candidate/membership', 'premium');
                    await loadCandidateMembershipPage();
                };
            }

            if (freeMarker) {
                freeMarker.onclick = async () => {
                    if (!isPremium) return;
                    await updateMembershipStatus('/api/candidate/membership', 'free');
                    await loadCandidateMembershipPage();
                };
                freeMarker.textContent = isPremium ? 'Downgrade to Free' : 'Current Plan';
            }
        } catch {
        }
    }

    async function loadEmployerMembershipPage() {
        if (!document.querySelector('#employer-membership-page-badge')) return;

        try {
            const response = await fetch('/api/employer/dashboard', { credentials: 'same-origin' });
            const data = await response.json().catch(() => ({}));
            if (!response.ok) return;

            applyPlanPresentation({
                status: data.membershipStatus || 'free',
                subject: 'candidate',
                badgeSelector: '#employer-membership-page-badge',
                titleSelector: '#employer-current-plan-title',
                copySelector: '#employer-current-plan-copy',
                freeMarkerSelector: '#employer-free-plan-marker',
                premiumButtonSelector: '#employer-premium-upgrade-button',
                primaryActionSelector: '#employer-membership-primary-action'
            });

            const premiumButton = document.querySelector('#employer-premium-upgrade-button');
            const primaryAction = document.querySelector('#employer-membership-primary-action');
            const freeMarker = document.querySelector('#employer-free-plan-marker');
            const isPremium = (data.membershipStatus || '').toLowerCase() === 'premium';

            if (premiumButton) {
                premiumButton.onclick = async () => {
                    if (premiumButton.disabled) return;
                    await updateMembershipStatus('/api/employer/membership', 'premium');
                    await loadEmployerMembershipPage();
                };
            }

            if (primaryAction) {
                primaryAction.onclick = async () => {
                    if (primaryAction.disabled) return;
                    await updateMembershipStatus('/api/employer/membership', 'premium');
                    await loadEmployerMembershipPage();
                };
            }

            if (freeMarker) {
                freeMarker.onclick = async () => {
                    if (!isPremium) return;
                    await updateMembershipStatus('/api/employer/membership', 'free');
                    await loadEmployerMembershipPage();
                };
                freeMarker.textContent = isPremium ? 'Downgrade to Free' : 'Current Plan';
            }
        } catch {
        }
    }

    // ============================================================
    // EMPLOYER CANDIDATES
    // ============================================================
    const employerCandidatesForm = document.querySelector('form[action="/employer/candidates"]');
    const employerCandidatesGrid = document.querySelector('#candidates-grid');

    function employerCandidateDetailsUrl(candidateId) {
        return `/employer/candidate?id=${encodeURIComponent(candidateId)}`;
    }

    function populateEmployerCandidatesFiltersFromUrl() {
        if (!employerCandidatesForm) return;

        const params = new URLSearchParams(window.location.search);
        employerCandidatesForm.querySelectorAll('[name]').forEach(field => {
            const value = params.get(field.name);
            if (value !== null) {
                field.value = value;
            }
        });
    }

    function syncEmployerCandidatesUrl() {
        if (!employerCandidatesForm) return;

        const params = new URLSearchParams();
        new FormData(employerCandidatesForm).forEach((value, key) => {
            const text = String(value).trim();
            if (text) {
                params.set(key, text);
            }
        });

        const query = params.toString();
        const targetUrl = query ? `/employer/candidates?${query}` : '/employer/candidates';
        window.history.replaceState({}, '', targetUrl);
    }

    async function loadEmployerCandidates() {
        if (!employerCandidatesForm || !employerCandidatesGrid) return;

        const params = new URLSearchParams(new FormData(employerCandidatesForm));

        try {
            const response = await fetch(`/api/employer/candidates?${params.toString()}`, { credentials: 'same-origin' });
            const data = await response.json().catch(() => ({}));
            if (!response.ok) return;

            const candidates = data.candidates || [];
            const shownCount = document.querySelector('#shownCount');
            const totalCount = document.querySelector('#totalCount');

            if (shownCount) shownCount.textContent = String(candidates.length);
            if (totalCount) totalCount.textContent = String(candidates.length);

            employerCandidatesGrid.innerHTML = candidates.length
                ? candidates.map(candidate => `
            <a class="card" href="${employerCandidateDetailsUrl(candidate.id)}" style="display:block; text-decoration:none; color:inherit;">
              <div class="card-title">${candidate.fullName}</div>
              <div style="color:var(--text-muted); margin-bottom:8px;">${candidate.major}</div>
              <div style="font-size:0.9rem;">${candidate.skills}</div>
              <div style="margin-top:8px; font-size:0.9rem; color:var(--text-dark);">${candidate.summary || 'No profile summary provided.'}</div>
              <div style="font-size:0.85rem; color:var(--text-muted); margin-top:8px;">${candidate.preferredLocation} • ${candidate.preferredWorkMode} • ${candidate.experienceText}</div>
            </a>
          `).join('')
                : '<div class="card">No candidates found.</div>';
        } catch {
        }
    }

    if (employerCandidatesForm && employerCandidatesGrid) {
        populateEmployerCandidatesFiltersFromUrl();
        employerCandidatesForm.addEventListener('submit', e => {
            e.preventDefault();
            syncEmployerCandidatesUrl();
            loadEmployerCandidates();
        });
        loadEmployerCandidates();
    }

    async function loadEmployerRecommendedCandidatesPage() {
        const recommendationsGrid = document.querySelector('#recommendations-grid');
        const recommendationsForm = document.querySelector('form[action="/employer/recommended-candidates"]');
        if (!recommendationsGrid || !recommendationsForm) return;

        const params = new URLSearchParams(window.location.search);
        const jobIdParam = params.get('job_id');
        const jobSelect = recommendationsForm.querySelector('[name="job_id"]');

        // Determine the selected job ID and view type (specific job or overall recommendations)
        const selectedJobId = jobIdParam || jobSelect?.value || '';
        const isSpecificJobView = !!selectedJobId;

        // Update job ID selector
        if (jobSelect) {
            jobSelect.value = selectedJobId;
        }

        try {
            const recommendationsParams = new URLSearchParams();
            if (selectedJobId) {
                recommendationsParams.set('job_id', selectedJobId);
            }

            const [jobsResponse, recommendationsResponse, dashboardResponse] = await Promise.all([
                fetch('/api/employer/jobs', { credentials: 'same-origin' }),
                fetch(`/api/employer/recommendations?${recommendationsParams.toString()}`, { credentials: 'same-origin' }),
                fetch('/api/employer/dashboard', { credentials: 'same-origin' })
            ]);

            const jobsData = await jobsResponse.json().catch(() => ({}));
            const recommendationsData = await recommendationsResponse.json().catch(() => ({}));
            const dashboardData = await dashboardResponse.json().catch(() => ({}));
            if (!jobsResponse.ok || !recommendationsResponse.ok || !dashboardResponse.ok) return;

            const jobs = jobsData.jobs || [];
            const recommendations = recommendationsData.recommendations || [];
            const isFreePlan = (dashboardData.membershipStatus || '').toLowerCase() === 'free';
            const visibleCount = document.querySelector('#visible-count');
            const totalCount = document.querySelector('#total-count');

            if (visibleCount) visibleCount.textContent = String(recommendations.length);
            if (totalCount) totalCount.textContent = String(recommendations.length);

            // Populate job selector for filtering recommendations
            if (jobSelect) {
                const currentValue = jobSelect.value;
                jobSelect.innerHTML = '<option value="">All recommended matches</option>' + jobs.map(job => `
          <option value="${job.id}">${job.title}</option>
        `).join('');
                jobSelect.value = currentValue;
            }

            // Render recommendations
            recommendationsGrid.innerHTML = recommendations.length
                ? recommendations.map(recommendation => renderEmployerRecommendationCard(recommendation, { showMatchedJobs: !isSpecificJobView })).join('')
                : '<div class="card">No recommended candidates found.</div>';
        } catch {
            recommendationsGrid.innerHTML = '<div class="card">Unable to load recommended candidates.</div>';
        }

        recommendationsForm.addEventListener('submit', e => {
            e.preventDefault();
            const nextParams = new URLSearchParams();
            const jobId = recommendationsForm.querySelector('[name="job_id"]')?.value || '';
            if (jobId) {
                nextParams.set('job_id', jobId);
            }
            const query = nextParams.toString();
            window.history.replaceState({}, '', query ? `/employer/recommended-candidates?${query}` : '/employer/recommended-candidates');
            loadEmployerRecommendedCandidatesPage();
        }, { once: true });
    }

    async function loadEmployerCandidateDetails() {
        const name = document.querySelector('#candidate-name');
        if (!name) return;

        const params = new URLSearchParams(window.location.search);
        const candidateId = params.get('id');
        if (!candidateId) return;

        const matchForm = document.querySelector('form[action="/employer/candidate_details"]');
        const hiddenCandidateId = matchForm?.querySelector('input[name="id"]');
        const jobSelect = matchForm?.querySelector('select[name="job_id"]');
        const selectedJobId = params.get('job_id') || '';

        if (hiddenCandidateId) {
            hiddenCandidateId.value = candidateId;
        }

        try {
            const jobsResponse = fetch('/api/employer/jobs', { credentials: 'same-origin' });
            const candidateResponse = fetch(`/api/employer/candidate?id=${encodeURIComponent(candidateId)}${selectedJobId ? `&job_id=${encodeURIComponent(selectedJobId)}` : ''}`, { credentials: 'same-origin' });
            const [jobsResult, candidateResult] = await Promise.all([jobsResponse, candidateResponse]);
            const jobsData = await jobsResult.json().catch(() => ({}));
            const data = await candidateResult.json().catch(() => ({}));
            if (!jobsResult.ok || !candidateResult.ok || !data.candidate) return;

            if (jobSelect) {
                const jobs = jobsData.jobs || [];
                jobSelect.innerHTML = '<option value="">Select a job posting…</option>' + jobs.map(job => `
                    <option value="${job.id}">${job.title}</option>
                `).join('');
                jobSelect.value = selectedJobId;
            }

            if (matchForm && !matchForm.dataset.bound) {
                matchForm.addEventListener('submit', e => {
                    e.preventDefault();
                    const nextParams = new URLSearchParams();
                    nextParams.set('id', candidateId);
                    const selected = jobSelect?.value || '';
                    if (selected) {
                        nextParams.set('job_id', selected);
                    }
                    window.location.href = `/employer/candidate?${nextParams.toString()}`;
                });
                matchForm.dataset.bound = 'true';
            }

            const candidate = data.candidate;
            const setText = (selector, value, fallback = '') => {
                const el = document.querySelector(selector);
                if (el) el.textContent = value || fallback;
            };

            setText('#candidate-name', candidate.fullName, 'Candidate');
            setText('#candidate-summary-meta', `${candidate.major || 'Field not listed'} • ${candidate.experienceText || 'Experience not listed'}`);
            setText('#about-text', candidate.summary, 'No candidate summary provided.');
            setText('#education', candidate.education, 'Not specified');
            setText('#major', candidate.major, 'Not specified');
            setText('#experience', candidate.experienceText, 'Not specified');
            setText('#employment-type', candidate.employmentType, 'Not specified');
            setText('#work-mode', candidate.preferredWorkMode, 'Not specified');
            setText('#preferred-location', candidate.preferredLocation, 'Not specified');
            setText('#expected-salary', candidate.expectedSalary, 'Not specified');
            setText('#contact-info', candidate.contactInfo, 'Not specified');
            setText('#member-since', candidate.createdAt, 'Not specified');
            setText('#work-experience', candidate.workExperience, 'No work experience provided.');

            const workModeBadge = document.querySelector('#candidate-work-mode-badge');
            if (workModeBadge) {
                workModeBadge.textContent = candidate.preferredWorkMode || 'Work mode not specified';
            }

            const skillsContainer = document.querySelector('#skills-container');
            if (skillsContainer) {
                const skills = (candidate.skills || '').split(',').map(skill => skill.trim()).filter(Boolean);
                skillsContainer.innerHTML = skills.length
                    ? skills.map(skill => `<span class="skill-tag">${skill}</span>`).join('')
                    : '<span style="color:var(--text-muted);">No skills listed.</span>';
            }

            const matchScore = clampMatchScore(candidate.matchScore);
            setText('#candidate-match-score-circle', `${matchScore}%`, '0%');
            setText('#candidate-match-score-text', `${matchScore}%`, '0%');
            applyScoreCircleColor('#candidate-match-score-circle', matchScore);
            setText('#matched-job-label', candidate.matchedJobTitle ? `Best match job: ${candidate.matchedJobTitle}` : 'Best match job: Not available');
            setText('#matched-job-summary', candidate.matchedJobTitle
                ? 'This score compares the candidate against your highest scoring job listing.'
                : 'Post a job to see how this candidate matches against your openings.');

            const matchBar = document.querySelector('#candidate-match-score-bar');
            if (matchBar) {
                matchBar.style.width = `${matchScore}%`;
            }
            applyMatchBarColor('#candidate-match-score-bar', matchScore);

            const matchHighlights = document.querySelector('#match-highlights');
            if (matchHighlights) {
                const reasons = Array.isArray(candidate.matchReasons) ? candidate.matchReasons : [];
                matchHighlights.innerHTML = reasons.length
                    ? reasons.map(reason => `<li>${reason}</li>`).join('')
                    : '<li>No personalized match reasons available.</li>';
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
        } catch {
        }
    }

    // ============================================================
    // EMPLOYER JOBS LIST
    // ============================================================
    async function loadEmployerJobs() {
        const jobsList = document.querySelector('#jobs-list');
        if (!jobsList) return;

        try {
            const response = await fetch('/api/employer/jobs', { credentials: 'same-origin' });
            const data = await response.json().catch(() => ({}));
            if (!response.ok) return;

            const jobs = data.jobs || [];
            const jobCount = document.querySelector('#job-count');
            if (jobCount) jobCount.textContent = String(jobs.length);

            jobsList.innerHTML = jobs.length
                ? jobs.map(job => `
            <div class="card">
              <div class="card-title">${job.title}</div>
              <div style="color:var(--text-muted); margin-bottom:8px;">${job.company}</div>
              <div style="font-size:0.9rem; color:var(--text-muted);">${job.type} • ${job.status} • ${job.posted}</div>
            </div>
          `).join('')
                : '<div class="card">No jobs posted yet.</div>';
        } catch {
        }
    }

    // ============================================================
    // ADMIN DASHBOARD
    // ============================================================
    async function loadAdminDashboard() {
        if (!document.querySelector('#totalUsers') || !document.querySelector('#recentUsersTable')) return;

        try {
            const response = await fetch('/api/admin/dashboard', { credentials: 'same-origin' });
            const data = await response.json().catch(() => ({}));
            if (!response.ok) return;

            const setText = (selector, value) => {
                const el = document.querySelector(selector);
                if (el) el.textContent = value;
            };

            setText('#totalUsers', data.totalUsers ?? 0);
            setText('#totalCandidates', data.totalCandidates ?? 0);
            setText('#totalEmployers', data.totalEmployers ?? 0);
            setText('#premiumUsers', data.premiumUsers ?? 0);
            setText('#totalJobs', data.totalJobs ?? 0);
            setText('#openJobs', data.openJobs ?? 0);
            setText('#totalProfiles', data.totalProfiles ?? 0);
            setText('#totalCompanies', data.totalCompanies ?? 0);

            const recentUsersTable = document.querySelector('#recentUsersTable');
            const recentJobsTable = document.querySelector('#recentJobsTable');

            if (recentUsersTable) {
                const users = data.recentUsers || [];
                recentUsersTable.innerHTML = users.map(user => `
          <tr>
            <td>${user.full_name}</td>
            <td>${user.role}</td>
            <td>${user.membership_status}</td>
            <td>${user.created_at}</td>
          </tr>
        `).join('');
            }

            if (recentJobsTable) {
                const jobs = data.recentJobs || [];
                recentJobsTable.innerHTML = jobs.map(job => `
          <tr>
            <td>${job.title}</td>
            <td>${job.company}</td>
            <td>${job.status}</td>
            <td>${job.posted}</td>
          </tr>
        `).join('');
            }
        } catch {
        }
    }

    // ============================================================
    // ADMIN JOBS
    // ============================================================
    const adminJobsForm = document.querySelector('form[action="/admin/jobs"]');
    const adminJobsBody = document.querySelector('#jobs-table-body');

    async function loadAdminJobs() {
        if (!adminJobsForm || !adminJobsBody) return;

        const params = new URLSearchParams(new FormData(adminJobsForm));
        try {
            const response = await fetch(`/api/admin/jobs?${params.toString()}`, { credentials: 'same-origin' });
            const data = await response.json().catch(() => ({}));
            if (!response.ok) return;

            const jobs = data.jobs || [];
            const totalJobs = document.querySelector('#totalJobs');
            if (totalJobs) totalJobs.textContent = String(jobs.length);

            adminJobsBody.innerHTML = jobs.length
                ? jobs.map((job, index) => `
            <tr>
              <td>${index + 1}</td>
              <td>${job.title}</td>
              <td>${job.company}</td>
              <td>${job.type}</td>
              <td>${job.status}</td>
              <td>${job.deadline}</td>
              <td>${job.posted}</td>
              <td><button type="button" class="btn btn-secondary btn-sm">View</button></td>
            </tr>
          `).join('')
                : '<tr><td colspan="8" style="text-align:center; color:var(--text-muted); padding:40px;">No jobs found.</td></tr>';
        } catch {
        }
    }

    if (adminJobsForm && adminJobsBody) {
        adminJobsForm.addEventListener('submit', e => {
            e.preventDefault();
            loadAdminJobs();
        });
        loadAdminJobs();
    }

    // ============================================================
    // EMPLOYER COMPANY PROFILE -> JSON API
    // ============================================================
    const companyProfileForm = document.querySelector('#company-profile-form');

    async function loadCompanyProfile() {
        if (!companyProfileForm) return;

        try {
            const response = await fetch('/api/employer/company-profile', {
                method: 'GET',
                credentials: 'same-origin'
            });

            const data = await response.json().catch(() => ({}));
            if (!response.ok || !data.profile) return;

            const profile = data.profile;
            Object.entries(profile).forEach(([key, value]) => {
                const field = companyProfileForm.querySelector(`[name="${key}"]`);
                if (field && value != null) {
                    field.value = value;
                }
            });

            const submitButton = companyProfileForm.querySelector('button[type="submit"]');
            if (submitButton) {
                submitButton.textContent = '💾 Save Company Profile';
            }
        } catch {
        }
    }

    if (companyProfileForm) {
        loadCompanyProfile();

        companyProfileForm.addEventListener('submit', async e => {
            e.preventDefault();

            const successBox = document.querySelector('#success-message');
            const errorBox = document.querySelector('#error-message');
            const submitButton = companyProfileForm.querySelector('button[type="submit"]');

            if (successBox) {
                successBox.hidden = true;
                successBox.textContent = '';
            }

            if (errorBox) {
                errorBox.hidden = true;
                errorBox.textContent = '';
            }

            const payload = {
                company_name: companyProfileForm.querySelector('[name="company_name"]')?.value.trim() || '',
                company_email: companyProfileForm.querySelector('[name="company_email"]')?.value.trim() || '',
                company_phone: companyProfileForm.querySelector('[name="company_phone"]')?.value.trim() || '',
                industry: companyProfileForm.querySelector('[name="industry"]')?.value || 'Technology',
                company_size: companyProfileForm.querySelector('[name="company_size"]')?.value || '11-50',
                company_location: companyProfileForm.querySelector('[name="company_location"]')?.value.trim() || '',
                company_description: companyProfileForm.querySelector('[name="company_description"]')?.value.trim() || '',
                website_url: companyProfileForm.querySelector('[name="website_url"]')?.value.trim() || ''
            };

            if (!payload.company_name) {
                if (errorBox) {
                    errorBox.textContent = 'Company name is required.';
                    errorBox.hidden = false;
                }
                return;
            }

            try {
                if (submitButton) submitButton.disabled = true;

                const response = await fetch('/api/employer/company-profile', {
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
                        errorBox.textContent = data.error || 'Failed to save company profile.';
                        errorBox.hidden = false;
                    }
                    return;
                }

                if (successBox) {
                    successBox.textContent = 'Company profile saved successfully.';
                    successBox.hidden = false;
                }

                window.location.href = '/employer/dashboard';
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

    loadCandidateDashboard();
    loadCandidateMembershipPage();
    loadCandidateJobDetails();
    loadRecommendedJobsPage();
    loadEmployerDashboard();
    loadEmployerMembershipPage();
    loadEmployerRecommendedCandidatesPage();
    loadEmployerCandidateDetails();
    loadEmployerJobs();
    loadAdminDashboard();

})();

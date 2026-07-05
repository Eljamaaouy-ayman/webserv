// Adjust this to match whatever cookie your server sets on login
const SESSION_COOKIE_NAME = 'session_id';

function isLoggedIn() {
  return document.cookie
    .split(';')
    .some(c => c.trim().startsWith(SESSION_COOKIE_NAME + '='));
}

function applyAuthState() {
  const loggedIn = isLoggedIn();

  document.querySelectorAll('[data-auth="guest"]').forEach(el => {
    el.style.display = loggedIn ? 'none' : '';
  });

  document.querySelectorAll('[data-auth="user"]').forEach(el => {
    el.style.display = loggedIn ? '' : 'none';
  });
}

async function logout() {
  try {
    await fetch('/logout', { method: 'POST' });
  } catch (e) {
    // ignore network errors, still clear client state below
  }
  document.cookie = SESSION_COOKIE_NAME + '=; expires=Thu, 01 Jan 1970 00:00:00 UTC; path=/;';
  window.location.href = 'index.html';
}

document.addEventListener('DOMContentLoaded', () => {
  applyAuthState();
  const logoutBtn = document.getElementById('logoutLink');
  if (logoutBtn) {
    logoutBtn.addEventListener('click', (e) => {
      e.preventDefault();
      logout();
    });
  }
});

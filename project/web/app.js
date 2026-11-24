/*
 * File: app.js
 * Project: Crypto Messenger
 * Version: 0.1.0
 * Updated: 2026-11-23
 * Description: Handles localization and web API interactions.
 */
const state = { payload: "" };
let dictionaries = {};

async function loadLocale(lang) {
  const response = await fetch(`/locales/${lang}.txt`, { cache: "no-store" });
  if (!response.ok) throw new Error("Unable to load language file.");
  const parsed = {};
  for (const line of (await response.text()).split(/\r?\n/)) {
    const trimmed = line.trim();
    if (!trimmed || trimmed.startsWith("#")) continue;
    const index = trimmed.indexOf("=");
    if (index > 0) parsed[trimmed.slice(0, index).trim()] = trimmed.slice(index + 1).trim();
  }
  dictionaries[lang] = parsed;
  document.documentElement.lang = lang;
  document.documentElement.dir = lang === "fa" ? "rtl" : "ltr";
  document.querySelectorAll("[data-i18n]").forEach(el => {
    const key = el.dataset.i18n;
    if (parsed[key]) el.textContent = parsed[key];
  });
  document.querySelectorAll("[data-i18n-placeholder]").forEach(el => {
    const key = el.dataset.i18nPlaceholder;
    if (parsed[key]) el.placeholder = parsed[key];
  });
}

function message(text, isError = false) {
  const el = document.getElementById("status");
  el.textContent = text;
  el.style.color = isError ? "#ff9a9a" : "var(--accent2)";
}

async function api(path, data) {
  const response = await fetch(path, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(data)
  });
  const result = await response.json();
  if (!response.ok) throw new Error(result.error || "Request failed.");
  return result;
}

document.querySelectorAll(".tab").forEach(button => {
  button.addEventListener("click", () => {
    document.querySelectorAll(".tab").forEach(item => item.classList.toggle("active", item === button));
    document.getElementById("encrypt-panel").classList.toggle("hidden", button.dataset.tab !== "encrypt");
    document.getElementById("decrypt-panel").classList.toggle("hidden", button.dataset.tab !== "decrypt");
    message("");
  });
});

document.getElementById("language").addEventListener("change", event => {
  loadLocale(event.target.value).catch(error => message(error.message, true));
});

document.getElementById("encrypt-btn").addEventListener("click", async () => {
  const text = document.getElementById("plain").value;
  const password = document.getElementById("password").value;
  const confirm = document.getElementById("confirm").value;
  if (!text) return message("Enter a message first.", true);
  if (password.length < 8) return message("Use a passphrase with at least 8 characters.", true);
  if (password !== confirm) return message("Passphrases do not match.", true);
  try {
    const result = await api("/api/encrypt", { text, password });
    state.payload = result.payload;
    document.getElementById("encrypted-output").value = state.payload;
    message("Encryption completed.");
  } catch (error) { message(error.message, true); }
});

document.getElementById("copy-btn").addEventListener("click", async () => {
  const value = document.getElementById("encrypted-output").value;
  if (!value) return message("Nothing to copy.", true);
  await navigator.clipboard.writeText(value);
  message("Encrypted text copied.");
});

document.getElementById("download-btn").addEventListener("click", () => {
  const value = document.getElementById("encrypted-output").value;
  if (!value) return message("Nothing to download.", true);
  const blob = new Blob([value], { type: "text/plain;charset=utf-8" });
  const url = URL.createObjectURL(blob);
  const anchor = document.createElement("a");
  anchor.href = url;
  anchor.download = "message.cmsg";
  anchor.click();
  URL.revokeObjectURL(url);
});

document.getElementById("save-btn").addEventListener("click", async () => {
  const payload = document.getElementById("encrypted-output").value;
  if (!payload) return message("Nothing to save.", true);
  try {
    await api("/api/save", { payload });
    message("Encrypted payload saved to SQLite.");
  } catch (error) { message(error.message, true); }
});

document.getElementById("decrypt-btn").addEventListener("click", async () => {
  const payload = document.getElementById("payload-input").value.trim();
  const password = document.getElementById("decrypt-password").value;
  if (!payload) return message("Paste an encrypted payload or load a file.", true);
  try {
    const result = await api("/api/decrypt", { payload, password });
    document.getElementById("plain-output").value = result.text;
    message("Decryption completed.");
  } catch (error) { message(error.message, true); }
});

document.getElementById("file-input").addEventListener("change", async event => {
  const file = event.target.files[0];
  if (!file) return;
  if (file.size > 2 * 1024 * 1024) return message("File is too large for this prototype.", true);
  document.getElementById("payload-input").value = (await file.text()).trim();
  message("CMSG file loaded. Enter the passphrase to decrypt.");
});

loadLocale("en").catch(error => message(error.message, true));

/*==== End of file app.js ====*/

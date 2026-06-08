const crypto = require('crypto');

// Single source of truth for password hashing, shared by the seed
// (database.js) and the auth/user routes (index_sqlite.js) so they can
// never drift apart. SHA-256 hex digest.
//
// NOTE: SHA-256 is fast, which makes it weak against brute-force/GPU
// cracking. For stronger storage, swap this one function over to bcrypt
// or argon2 (adds a dependency, and verification becomes async). Doing it
// here keeps the change to a single place.
function hashPassword(password) {
  return crypto.createHash('sha256').update(password).digest('hex');
}

module.exports = { hashPassword };

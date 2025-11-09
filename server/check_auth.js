const crypto = require('crypto');
const db = require('better-sqlite3')('./restaurant.db');

function hashPassword(password) {
  return crypto.createHash('sha256').update(password).digest('hex');
}

console.log('========================================');
console.log('🔐 Checking Authentication');
console.log('========================================\n');

const users = db.prepare('SELECT username, password, role FROM users').all();

console.log('📋 Users in database:');
users.forEach(user => {
  console.log(`\n  Username: ${user.username}`);
  console.log(`  Role: ${user.role}`);
  console.log(`  Stored Hash: ${user.password}`);
});

console.log('\n========================================');
console.log('🔑 Testing Password Hashes');
console.log('========================================\n');

const testPasswords = [
  { username: 'admin', password: 'admin123' },
  { username: 'staff', password: 'staff123' }
];

testPasswords.forEach(test => {
  const hash = hashPassword(test.password);
  const user = users.find(u => u.username === test.username);
  
  console.log(`\n${test.username}:`);
  console.log(`  Password: ${test.password}`);
  console.log(`  Generated Hash: ${hash}`);
  
  if (user) {
    console.log(`  Stored Hash:    ${user.password}`);
    console.log(`  Match: ${hash === user.password ? '✅ YES' : '❌ NO'}`);
  } else {
    console.log(`  ❌ User not found in database`);
  }
});

console.log('\n========================================\n');

db.close();

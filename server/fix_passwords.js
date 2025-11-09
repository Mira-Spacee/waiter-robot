const crypto = require('crypto');
const db = require('better-sqlite3')('./restaurant.db');

function hashPassword(password) {
  return crypto.createHash('sha256').update(password).digest('hex');
}

console.log('========================================');
console.log('🔧 Fixing Password Hashes');
console.log('========================================\n');

// Hash the passwords properly
const adminHash = hashPassword('admin123');
const staffHash = hashPassword('staff123');

console.log('Generated hashes:');
console.log(`  admin123 → ${adminHash}`);
console.log(`  staff123 → ${staffHash}`);

console.log('\nUpdating database...');

try {
  // Update admin password
  db.prepare('UPDATE users SET password = ? WHERE username = ?').run(adminHash, 'admin');
  console.log('✅ Updated admin password');
  
  // Update staff password
  db.prepare('UPDATE users SET password = ? WHERE username = ?').run(staffHash, 'staff');
  console.log('✅ Updated staff password');
  
  console.log('\n========================================');
  console.log('✅ Password hashes fixed!');
  console.log('========================================');
  console.log('\nYou can now login with:');
  console.log('  Admin: username=admin, password=admin123');
  console.log('  Staff: username=staff, password=staff123');
  console.log('========================================\n');
  
} catch (error) {
  console.error('❌ Error updating passwords:', error);
}

db.close();

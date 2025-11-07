# 🔐 Authentication & User Management Guide

## Overview
Your restaurant management system now has a complete authentication system with secure password hashing and role-based access control.

---

## 🚀 Quick Start

### Default Admin Account
- **Username:** `admin`
- **Password:** `admin123`
- **Role:** Admin

⚠️ **IMPORTANT:** This account was created automatically on first server startup. Please change the password after your first login by deleting this user and creating a new admin account with a strong password.

---

## 👥 User Roles

### Admin
- Access to Admin Reports (sales analytics, charts)
- Full access to User Management (add/delete users)
- Can create both Admin and Staff accounts
- Cannot delete the last admin account (safety feature)

### Staff
- Access to Staff Dashboard (order management)
- Can mark orders as Paid or Sent
- Cannot access Admin features

---

## 🔑 Login Pages

### Admin Login
- URL: `http://YOUR_IP:8080/admin/login`
- Beautiful purple gradient design with Shield icon
- Access to Admin Reports and User Management

### Staff Login
- URL: `http://YOUR_IP:8080/staff/login`
- Beautiful blue gradient design with Users icon
- Access to Staff Dashboard for order management

---

## 👤 User Management (Admin Only)

### Access User Management
1. Login as Admin
2. Click **"Manage Users"** button in Admin Reports header
3. Or navigate to: `http://YOUR_IP:8080/admin/users`

### Add New User
1. Click **"Add User"** button
2. Fill in the form:
   - **Username:** Unique identifier (no spaces)
   - **Password:** Strong password (min 8 characters recommended)
   - **Role:** Select Admin or Staff
3. Click **"Create User"**

### Delete User
1. Find the user in the table
2. Click **"Delete"** button next to their name
3. Confirm the deletion
4. ⚠️ You cannot delete the last admin account

### User Table Columns
- **Username:** Display name with role indicator (purple dot = admin, blue dot = staff)
- **Role:** Badge showing ADMIN (purple gradient) or STAFF (blue gradient)
- **Created Date:** When the account was created
- **Actions:** Delete button (red)

---

## 🔒 Security Features

### Password Hashing
- All passwords are hashed using **SHA-256** before storage
- Original passwords are never stored in plain text
- Example hash: `240be518fabd2724ddb6f04eeb1da5967448d7e831c08c8fa822809f74c720a9`

### Role-Based Access Control
- Login endpoints verify both username, password, AND role
- Trying to login with wrong role will fail (e.g., admin credentials at staff login)
- Session storage tracks authenticated users

### Safety Features
- Duplicate username prevention (409 Conflict response)
- Last admin deletion prevention (400 Bad Request response)
- Invalid role rejection (400 Bad Request response)
- Connection error handling with user-friendly messages

---

## 📡 API Endpoints

### Authentication
```
POST /api/auth/login
Body: { username, password, role }
Response: { id, username, role, createdAt }
```

### User Management
```
GET /api/users
Response: Array of users (without passwords)

POST /api/users
Body: { username, password, role }
Response: { id, username, role, createdAt }

DELETE /api/users/:id
Response: { message: "User deleted successfully" }
```

---

## 📁 Data Storage

### Location
`server/users.json`

### Structure
```json
[
  {
    "id": "1",
    "username": "admin",
    "password": "240be518fabd2724ddb6f04eeb1da5967448d7e831c08c8fa822809f74c720a9",
    "role": "admin",
    "createdAt": "2025-10-26T13:08:33.049Z"
  }
]
```

### Backup
- Keep regular backups of `users.json`
- If deleted, the server will recreate default admin on restart

---

## 🎨 Design Features

### Login Pages
- Modern glassmorphism design
- Gradient backgrounds (purple for admin, blue for staff)
- Animated icons (Shield for admin, Users for staff)
- Loading states during authentication
- Error messages with toast notifications

### User Management Page
- Professional table layout
- Gradient role badges
- Modal form for adding users
- Confirmation dialogs for deletions
- Real-time user count display
- Responsive design for mobile/tablet

---

## 🧪 Testing the System

### Test Admin Login
1. Navigate to `http://YOUR_IP:8080/admin/login`
2. Enter username: `admin`, password: `admin123`
3. Click Login
4. Should redirect to Admin Reports

### Test Create Staff User
1. Login as admin
2. Go to User Management
3. Create new user:
   - Username: `john`
   - Password: `john1234`
   - Role: Staff
4. Click Create User
5. New user should appear in table

### Test Staff Login
1. Navigate to `http://YOUR_IP:8080/staff/login`
2. Enter username: `john`, password: `john1234`
3. Click Login
4. Should redirect to Staff Dashboard

### Test Security Features
1. Try logging in with wrong password → Should show error
2. Try logging in with admin credentials at staff login → Should fail
3. Try creating duplicate username → Should show error
4. Try deleting last admin → Should show error

---

## 🔧 Troubleshooting

### "Error connecting to server"
- Check if backend is running: `node server/index.js`
- Verify backend is on port 3001
- Check firewall settings

### "Invalid credentials"
- Verify username is correct (case-sensitive)
- Verify password is correct
- Verify you're logging in at correct page (admin vs staff)
- Check role matches the login page

### "Failed to create user"
- Username may already exist
- Role may be invalid (must be 'admin' or 'staff')
- Check backend console for detailed errors

### "Cannot delete user"
- You're trying to delete the last admin account
- Create another admin first, then delete

---

## 📝 Best Practices

### Password Security
1. Use strong passwords (min 12 characters, mix of upper/lower/numbers/symbols)
2. Change default admin password immediately
3. Don't share passwords between users
4. Consider using a password manager

### User Management
1. Create individual accounts for each staff member
2. Remove accounts when staff leaves
3. Keep at least 2 admin accounts (backup)
4. Regularly audit user list

### Access Control
1. Only give admin access to trusted managers
2. Give staff access to kitchen/service personnel
3. Don't share admin credentials with staff
4. Monitor login activity

---

## 🚀 Future Enhancements (Optional)

### Potential Improvements
- Password strength requirements
- Password change functionality
- User activity logs
- Session timeout
- Two-factor authentication (2FA)
- Email verification
- Password reset via email
- Account suspension (instead of deletion)

---

## 📞 Support

If you encounter issues:
1. Check backend console for error messages
2. Check browser console (F12) for frontend errors
3. Verify `users.json` file exists and is valid JSON
4. Restart backend server if needed
5. Clear browser cache and sessionStorage

---

## ✅ Checklist

- [x] Backend API with authentication endpoints
- [x] Password hashing (SHA-256)
- [x] Role-based access control
- [x] Beautiful modern login pages
- [x] User Management UI
- [x] Add/Delete user functionality
- [x] Security features (duplicate prevention, last admin protection)
- [x] Default admin account creation
- [x] Auto IP detection for WiFi/hotspot

**Your restaurant system is now production-ready with professional authentication!** 🎉

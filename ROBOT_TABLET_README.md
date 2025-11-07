# 🤖 AVIV Robot Tablet Interface

## Overview
The Robot Tablet Interface is designed to be displayed on a tablet mounted on the AVIV delivery robot. It provides a cute, interactive interface that guides the robot through the delivery process.

## Features

### 1. **Three Robot States**

#### State 1: Traveling 🚀
- Shows the AVIV logo with a sun icon
- Displays cute animated emoji faces that cycle through
- Shows "On my way to deliver..." message
- Displays target table numbers
- Animated loading dots

#### State 2: Arrived 🎉
- Shows arrival animation with happy emoji
- Displays complete order information
- Shows items, quantities, and prices for each table
- Large "DONE" button for customers to confirm pickup

#### State 3: Completed ✓
- Thank you message with cute emoji
- "Returning to base..." message
- Auto-closes the window after 2 seconds

### 2. **Cute Emoji Rotation**
The interface displays these adorable text-based robot faces:
- `(◕‿◕)`
- `(づ｡◕‿‿◕｡)づ`
- `(ﾉ◕ヮ◕)ﾉ*:･ﾟ✧`
- `(◠‿◠)`
- `(✿◠‿◠)`
- `(◕ᴗ◕✿)`
- `(｡♥‿♥｡)`
- `(◕‿◕✿)`

## How to Use

### From Staff Dashboard:

1. **Mark orders as PAID** (this is required first)
2. Click the **"LAUNCH AVIV!"** button at the bottom of the staff dashboard
3. The robot tablet interface will open in a new window/tab
4. The robot begins its delivery mission

### Robot Workflow:

1. **Staff clicks "Launch AVIV!"**
   - Orders are sent to the robot
   - Tablet shows traveling state with cute animations
   
2. **Robot travels to table**
   - Cute emoji faces animate
   - Shows which table(s) it's delivering to
   
3. **Robot arrives at table** (currently simulated at 10 seconds)
   - Shows complete order details
   - Displays "DONE" button
   
4. **Customer clicks "DONE"**
   - Confirms order pickup
   - Robot returns to base
   - Window closes automatically

## Technical Details

### Data Storage
- Mission data is stored in `localStorage` with key `robotMission`
- Data structure:
```json
{
  "tables": [1, 2],
  "orders": [...],
  "timestamp": "2025-11-05T..."
}
```

### Integration Points

#### Current Implementation:
- **Simulated arrival**: Uses a 10-second timeout to simulate robot reaching the table

#### Future ESP32 Integration:
Replace the arrival simulation with actual ESP32 communication:

```typescript
// Replace this in RobotTablet.tsx:
const arrivalTimeout = setTimeout(() => {
  setRobotState('arrived');
}, 10000);

// With ESP32 WebSocket or HTTP polling:
const checkRobotStatus = async () => {
  const response = await fetch('http://ESP32_IP/robot-status');
  const data = await response.json();
  if (data.arrived) {
    setRobotState('arrived');
  }
};
```

### Staff Interface Changes

#### Table Refresh Logic:
- Tables now only refresh when **BOTH** "Paid" and "Sent" buttons are clicked
- Previous behavior: Clicking "Sent" alone would clear the table
- New behavior: Must click "Paid" first, then "Sent" to clear

#### Launch AVIV Button:
- Beautiful animated button with gradient and hover effects
- Only launches if there are paid orders
- Shows error toast if no orders are ready
- Opens robot tablet in new window

## Customization

### Adjust Arrival Time (for testing):
In `src/pages/RobotTablet.tsx`, line ~30:
```typescript
const arrivalTimeout = setTimeout(() => {
  setRobotState('arrived');
}, 10000); // Change this value (in milliseconds)
```

### Change Emoji Speed:
In `src/pages/RobotTablet.tsx`, line ~25:
```typescript
const emojiInterval = setInterval(() => {
  setCurrentEmoji((prev) => (prev + 1) % cuteEmojis.length);
}, 800); // Change this value (in milliseconds)
```

### Add More Emojis:
In `src/pages/RobotTablet.tsx`, lines 11-20, add to the array:
```typescript
const cuteEmojis = [
  '(◕‿◕)',
  '(づ｡◕‿‿◕｡)づ',
  // Add more here!
];
```

## AVIV Logo

Place your AVIV logo image in `/public/aviv-logo.png`

The logo should be:
- PNG format with transparent background
- Recommended size: 512x512 pixels or larger
- If no logo is provided, a text-based version is shown

## Routes

- **Robot Tablet**: `/robot-tablet`
- **Staff Dashboard**: `/staff/dashboard`

## Color Scheme

The interface uses a beautiful gradient color scheme:
- **Blue** → **Indigo** → **Purple** (traveling state)
- **Green** → **Emerald** (arrived/success state)
- Soft pastel backgrounds for a friendly appearance

## Future Enhancements (planned)

1. **ESP32 Integration**
   - Real-time robot position tracking
   - Actual arrival detection
   - Battery status display

2. **Multi-language Support**
   - English, Hebrew, etc.

3. **Voice Notifications**
   - "Your order has arrived!"
   - Text-to-speech announcements

4. **Camera Feed**
   - Show robot's camera view during travel

5. **Table Confirmation**
   - QR code scanning to verify correct table

## Troubleshooting

### Robot tablet doesn't open:
- Check if browser is blocking pop-ups
- Verify the route `/robot-tablet` is accessible

### Orders don't show:
- Ensure orders are marked as "Paid" before launching
- Check browser console for errors
- Verify localStorage has `robotMission` data

### Logo doesn't appear:
- Place `aviv-logo.png` in the `/public` folder
- Check image path and format
- Text-based logo will show as fallback

## Testing

1. Go to Staff Dashboard
2. Create some orders for different tables
3. Mark orders as "Paid"
4. Click "Launch AVIV!"
5. Watch the cute animations!
6. Wait 10 seconds for simulated arrival
7. Click "DONE" to complete

Enjoy your AVIV robot! 🤖✨

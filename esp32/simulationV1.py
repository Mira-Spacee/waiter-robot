"""
MODULE 3 Table Navigation Simulator
Python version that matches the ESP32 greedy algorithm exactly
"""

import time
import sys

# ============================================== SETTINGS /////////////////

SIM_SPEED = 0.2  # Seconds per marker movement (adjust for faster/slower animation)

# Distance matrix (staff = 0, tables = 1-10) - EXACT COPY FROM MODULE 3
DISTANCE_MATRIX = [
    #   0   1   2   3   4   5   6   7   8   9  10
    [   0, 20, 19, 17, 16, 15, 14, 13, 10,  9,  8 ],  # Staff
    [  20,  0,  1,  3,  4,  5,  6,  7, 10, 11, 12 ],  # T1
    [  19,  1,  0,  2,  3,  4,  5,  6,  9, 10, 11 ],  # T2
    [  17,  3,  2,  0,  1,  2,  3,  4,  7,  8,  9 ],  # T3
    [  16,  4,  3,  1,  0,  1,  2,  3,  6,  7,  8 ],  # T4
    [  15,  5,  4,  2,  1,  0,  1,  2,  5,  6,  7 ],  # T5
    [  14,  6,  5,  3,  2,  1,  0,  1,  4,  5,  6 ],  # T6
    [  13,  7,  6,  4,  3,  2,  1,  0,  3,  4,  5 ],  # T7
    [  10, 10,  9,  7,  6,  5,  4,  3,  0,  1,  2 ],  # T8
    [   9, 11, 10,  8,  7,  6,  5,  4,  1,  0,  1 ],  # T9
    [   8, 12, 11,  9,  8,  7,  6,  5,  2,  1,  0 ]   # T10
]

# ============================================== GLOBAL VARIABLES /////////////////

order_list = []       # Original order list (dynamically shrinks as orders served)
order_served = [False] * 11  # Track which tables have been served (0=staff, 1-10=tables)
served_count = 0      # Number of orders served
current_position = 0  # Current position (0=staff, 1-10=tables)
target_table = 0      # Current target table
cumulative_counter = 0  # Cumulative IR5 count (always increments)

# ============================================== GREEDY ALGORITHM /////////////////

def decide_direction():
    """
    Decide direction based on reachability + opportunistic serving
    FORWARD: +5 tables reach
    BACKWARD: +4 tables reach (with wrapping)
    Returns: (is_forward, target_table, calc_time_us)
    """
    global current_position
    
    start_time = time.perf_counter()
    
    unserved = [t for t in order_list if not order_served[t]]
    
    if not unserved:
        return None, -1, 0
    
    forward_reachable = []
    backward_reachable = []
    
    # Check FORWARD reachability (current+1 to current+5)
    for table in unserved:
        if table > current_position and table <= current_position + 5:
            forward_reachable.append(table)
    
    # Check BACKWARD reachability (4 steps back with wrapping)
    for table in unserved:
        # Calculate backward distance with wrapping
        if table < current_position:
            backward_steps = current_position - table
        else:
            # Wrapping: current→...→1→10→9→...→table
            backward_steps = current_position + (10 - table)
        
        if backward_steps > 0 and backward_steps <= 4:
            backward_reachable.append(table)
    
    # Choose direction with more reachable tables
    if len(forward_reachable) > len(backward_reachable):
        target = max(forward_reachable)  # Farthest forward
        is_forward = True
    elif len(backward_reachable) > len(forward_reachable):
        # Farthest backward = the one with most steps
        farthest = backward_reachable[0]
        max_steps = current_position - farthest if farthest < current_position else current_position + (10 - farthest)
        for table in backward_reachable[1:]:
            steps = current_position - table if table < current_position else current_position + (10 - table)
            if steps > max_steps:
                max_steps = steps
                farthest = table
        target = farthest
        is_forward = False
    elif len(forward_reachable) > 0:
        # Equal: prefer forward
        target = max(forward_reachable)
        is_forward = True
    else:
        # No reachable (shouldn't happen)
        target = current_position + 1
        is_forward = True
    
    calc_time = (time.perf_counter() - start_time) * 1000000  # microseconds
    
    return is_forward, target, calc_time

# ============================================== SIMULATION /////////////////

def simulate_navigation():
    """
    Simulate range-based F/B navigation with opportunistic serving
    """
    global current_position, served_count, order_served
    
    if not order_list:
        print("❌ No orders to process!")
        return
    
    print("\n" + "=" * 60)
    print("🚀 STARTING REACHABILITY-BASED NAVIGATION")
    print("=" * 60)
    print("Strategy: Forward +5 reach vs Backward +4 reach (wrap),")
    print("          choose direction with more tables, serve opportunistically!")
    print("=" * 60)
    
    # Reset state
    current_position = 0
    served_count = 0
    order_served = [False] * 11
    total_markers = 0
    cumulative_counter = 0
    
    # Save original order list (since we'll remove items during serving)
    original_order_count = len(order_list)
    
    print(f"\n📍 Starting at: Staff Station")
    print()
    
    start_time = time.perf_counter()
    
    # Save original order count (since we'll remove items during serving)
    original_order_count = len(order_list)
    
    # Main navigation loop
    while len(order_list) > 0:
        # Reachability decision
        is_forward, target_table, calc_time = decide_direction()
        
        if target_table == -1:
            break  # All served
        
        distance = DISTANCE_MATRIX[current_position][target_table]
        
        print(f"🎯 Target: Table {target_table} ({distance} markers away)")
        print(f"⚡ Calculation time: {calc_time:.1f} μs")
        
        # Determine direction
        moving_forward = is_forward
        direction = "FORWARD (+5 reach)" if moving_forward else "BACKWARD (+4 reach, wrap)"
        print(f"📍 Direction: {direction} from T{current_position}")
        print()
        
        # Move toward target, checking each table along the way
        if moving_forward:
            for table_num in range(current_position + 1, target_table + 1):
                if table_num == 1 and current_position == 0:
                    steps = DISTANCE_MATRIX[0][1]
                else:
                    steps = DISTANCE_MATRIX[table_num - 1][table_num]
                
                sys.stdout.write(f"   Moving: T{current_position} → T{table_num} ")
                for _ in range(steps):
                    sys.stdout.write("▓")
                    sys.stdout.flush()
                    time.sleep(SIM_SPEED)
                print(" ✓")
                
                total_markers += steps
                current_position = table_num
                
                # Opportunistic: Check if this table has an order
                if table_num in order_list and not order_served[table_num]:
                    print(f"   ⚡ OPPORTUNISTIC: Table {table_num} has an order!")
                    print(f"✅ Serving Table {table_num}")
                    print(f"🍽️  Serving order... (5 seconds)")
                    
                    for sec in range(5):
                        sys.stdout.write(f"\r   Waiting: {'█' * (sec + 1)}{'░' * (4 - sec)} {sec + 1}s/5s")
                        sys.stdout.flush()
                        time.sleep(1)
                    
                    order_served[table_num] = True
                    served_count += 1
                    
                    # Remove from order list
                    order_list.remove(table_num)
                    
                    print(f"\n✔️  Order served! Progress: {served_count} served, {len(order_list)} remaining")
                    print()
        else:
            # BACKWARD with wrapping: 0→10→9→8→7 or 5→4→3→2→1→10→9
            path = []
            pos = current_position
            
            # Build the backward path
            while pos != target_table:
                if pos == 0:
                    next_pos = 10
                elif pos == 1:
                    next_pos = 0
                else:
                    next_pos = pos - 1
                
                path.append(next_pos)
                pos = next_pos
            
            # Follow the path
            for table_num in path:
                if current_position == 0 and table_num == 10:
                    steps = DISTANCE_MATRIX[0][10]  # Staff to T10 backward
                elif table_num == 0:
                    steps = DISTANCE_MATRIX[1][0]  # T1 to Staff backward
                else:
                    steps = DISTANCE_MATRIX[table_num + 1][table_num]  # T(n+1) to Tn backward
                
                sys.stdout.write(f"   Moving: T{current_position} → T{table_num} ")
                for _ in range(steps):
                    sys.stdout.write("▓")
                    sys.stdout.flush()
                    time.sleep(SIM_SPEED)
                print(" ✓")
                
                total_markers += steps
                current_position = table_num
                
                # Opportunistic: Check if this table has an order
                if table_num in order_list and not order_served[table_num]:
                    print(f"   ⚡ OPPORTUNISTIC: Table {table_num} has an order!")
                    print(f"✅ Serving Table {table_num}")
                    print(f"🍽️  Serving order... (5 seconds)")
                    
                    for sec in range(5):
                        sys.stdout.write(f"\r   Waiting: {'█' * (sec + 1)}{'░' * (4 - sec)} {sec + 1}s/5s")
                        sys.stdout.flush()
                        time.sleep(1)
                    
                    order_served[table_num] = True
                    served_count += 1
                    
                    # Remove from order list
                    order_list.remove(table_num)
                    
                    print(f"\n✔️  Order served! Progress: {served_count} served, {len(order_list)} remaining")
                    print()
    
    # Smart return to staff: calculate optimal direction
    print(f"\n🎉 All orders served!")
    
    # Calculate steps needed in each direction
    forward_steps = (10 - current_position) + 1  # To T10, then wrap to Staff
    backward_steps = current_position  # Direct path back
    
    if forward_steps <= backward_steps:
        print(f"🔙 Returning FORWARD (needs {forward_steps} steps)")
        moving_forward = True
        # Continue incrementing
    else:
        print(f"🔙 Returning BACKWARD (needs {backward_steps} steps)")
        moving_forward = False
        cumulative_counter = 10 + current_position
    
    # Simulate return journey with counting
    if moving_forward:
        # Forward to staff: current→...10→0
        path = list(range(current_position + 1, 11)) + [0]
        for table_num in path:
            if table_num == 0:
                steps = DISTANCE_MATRIX[10][0]
            else:
                steps = DISTANCE_MATRIX[table_num - 1][table_num]
            
            sys.stdout.write(f"   Moving: T{current_position} → T{table_num if table_num > 0 else 'Staff'} ")
            for _ in range(steps):
                sys.stdout.write("▓")
                sys.stdout.flush()
                time.sleep(SIM_SPEED)
            print(" ✓")
            
            total_markers += steps
            current_position = table_num
    else:
        # Backward to staff: current→...→1→0
        for table_num in range(current_position - 1, -1, -1):
            if current_position == 1 and table_num == 0:
                steps = DISTANCE_MATRIX[1][0]
            else:
                steps = DISTANCE_MATRIX[table_num + 1][table_num] if table_num > 0 else DISTANCE_MATRIX[1][0]
            
            sys.stdout.write(f"   Moving: T{current_position} → T{table_num if table_num > 0 else 'Staff'} ")
            for _ in range(steps):
                sys.stdout.write("▓")
                sys.stdout.flush()
                time.sleep(SIM_SPEED)
            print(" ✓")
            
            total_markers += steps
            current_position = table_num
    
    calc_time = (time.perf_counter() - start_time) * 1000
    
    calc_time = (time.perf_counter() - start_time) * 1000
    
    print("\n" + "=" * 60)
    print("🏁 SIMULATION COMPLETE")
    print("=" * 60)
    print(f"✅ All {original_order_count} orders served successfully!")
    print(f"📊 Actual markers traveled: {total_markers}")
    print(f"⏱️  Total simulation time: {calc_time:.1f} ms")
    print(f"⏱️  Estimated real time: {total_markers * 2} seconds (at 2s per marker)")
    print()

# ============================================== USER INTERFACE /////////////////

def print_header():
    """Print welcome header"""
    print("\n" + "=" * 60)
    print("   MODULE 3 TABLE NAVIGATION SIMULATOR (Python)")
    print("=" * 60)
    print()
    print("🗺️  Restaurant Layout:")
    print("   Staff (IR1) → T1 → T2 → T3 → T4 → T5 → T6 → T7 → T8 → T9 → T10")
    print("   IR5 counts tables | IR1 detects staff station only")
    print()
    print("📋 Distance Matrix:")
    print("   - T1, T2 are FARTHEST (19-20 markers from staff)")
    print("   - T8, T9, T10 are CLOSEST (8-10 markers from staff)")
    print()
    print("🤖 Algorithm: Reachability + Opportunistic (same as ESP32 MODULE 3)")
    print("   1. Check reachable tables from current position:")
    print("      FORWARD: +5 tables (current+1 to current+5)")
    print("      BACKWARD: +4 tables with wrap (1→10→9...)")
    print("   2. Choose direction with MORE reachable tables")
    print("   3. Go to farthest reachable, serve opportunistically!")
    print()

def print_menu():
    """Print main menu"""
    print("=" * 60)
    print("COMMANDS:")
    print("=" * 60)
    print("  add <table>     - Add single order (e.g., 'add 5')")
    print("  add <t1,t2,...> - Add multiple orders (e.g., 'add 1,10,7')")
    print("  show            - Show current orders")
    print("  clear           - Clear all orders")
    print("  start           - Calculate path and run simulation")
    print("  test            - Show test scenarios")
    print("  speed <sec>     - Change simulation speed (default: 0.2)")
    print("  help            - Show this menu")
    print("  exit            - Quit simulator")
    print("=" * 60)
    print()

def show_current_orders():
    """Display current order list"""
    if not order_list:
        print("📋 No orders added yet.")
        print("   Use 'add <table>' to add orders (tables 1-10)")
    else:
        print(f"📋 Current orders ({len(order_list)} tables):")
        print(f"   {', '.join([f'T{t}' for t in order_list])}")
    print()

def add_order(table_input):
    """Add order(s) to the list"""
    global order_list
    
    # Handle comma-separated input
    if ',' in table_input:
        tables = [t.strip() for t in table_input.split(',')]
    else:
        tables = [table_input.strip()]
    
    added = []
    invalid = []
    
    for table_str in tables:
        try:
            table = int(table_str)
            if 1 <= table <= 10:
                order_list.append(table)
                added.append(table)
            else:
                invalid.append(table_str)
        except ValueError:
            invalid.append(table_str)
    
    if added:
        print(f"✅ Added orders: {', '.join([f'T{t}' for t in added])}")
    
    if invalid:
        print(f"❌ Invalid tables (must be 1-10): {', '.join(invalid)}")
    
    show_current_orders()

def clear_orders():
    """Clear all orders"""
    global order_list, optimized_path
    order_list = []
    optimized_path = []
    print("🗑️  All orders cleared!")
    print()

def show_test_scenarios():
    """Show suggested test scenarios"""
    print("\n" + "=" * 60)
    print("TEST SCENARIOS (copy and paste these):")
    print("=" * 60)
    print()
    print("1. Best Case (closest tables):")
    print("   add 10,9,8")
    print("   Expected: Staff → T10 → T9 → T8 → Staff (12 markers)")
    print()
    print("2. Worst Case (farthest tables):")
    print("   add 1,2,3")
    print("   Expected: Staff → T3 → T2 → T1 → Staff (27 markers)")
    print()
    print("3. Mixed Distance:")
    print("   add 1,10,7")
    print("   Expected: Staff → T10 → T7 → T1 → Staff (40 markers)")
    print()
    print("4. Greedy Optimization Demo:")
    print("   add 1,10,5")
    print("   Expected: Staff → T10 → T5 → T1 → Staff (38 markers)")
    print()
    print("5. Random Order:")
    print("   add 3,8,1,6,10")
    print("   Greedy will find optimal path!")
    print()
    print("6. All Tables:")
    print("   add 1,2,3,4,5,6,7,8,9,10")
    print("   Watch greedy algorithm optimize the full route!")
    print("=" * 60)
    print()

def change_speed(speed_str):
    """Change simulation speed"""
    global SIM_SPEED
    try:
        speed = float(speed_str)
        if 0.01 <= speed <= 5.0:
            SIM_SPEED = speed
            print(f"⚡ Simulation speed changed to {speed} seconds per marker")
        else:
            print("❌ Speed must be between 0.01 and 5.0 seconds")
    except ValueError:
        print("❌ Invalid speed value. Use: speed <number>")
    print()

# ============================================== MAIN /////////////////

def main():
    """Main program loop"""
    print_header()
    print_menu()
    
    while True:
        try:
            command = input("simulator> ").strip().lower()
            
            if not command:
                continue
            
            parts = command.split(maxsplit=1)
            cmd = parts[0]
            
            if cmd == "help":
                print_menu()
            
            elif cmd == "show":
                show_current_orders()
            
            elif cmd == "add":
                if len(parts) < 2:
                    print("❌ Usage: add <table> or add <t1,t2,...>")
                    print("   Example: add 5  or  add 1,10,7")
                else:
                    add_order(parts[1])
            
            elif cmd == "clear":
                clear_orders()
            
            elif cmd == "start":
                if not order_list:
                    print("❌ No orders to process! Use 'add <table>' first.")
                    print()
                else:
                    simulate_navigation()
            
            elif cmd == "test":
                show_test_scenarios()
            
            elif cmd == "speed":
                if len(parts) < 2:
                    print(f"⚡ Current speed: {SIM_SPEED} seconds per marker")
                    print("   Usage: speed <number>")
                else:
                    change_speed(parts[1])
            
            elif cmd == "exit" or cmd == "quit":
                print("\n👋 Goodbye! Happy robot testing!")
                break
            
            else:
                print(f"❌ Unknown command: '{cmd}'")
                print("   Type 'help' to see available commands")
                print()
        
        except KeyboardInterrupt:
            print("\n\n👋 Goodbye! Happy robot testing!")
            break
        except Exception as e:
            print(f"❌ Error: {e}")
            print()

if __name__ == "__main__":
    main()

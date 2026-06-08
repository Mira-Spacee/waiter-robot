"""
MODULE 3 Table Navigation Simulator
Python version that matches the ESP32 algorithm exactly

ALGORITHM:
- Single Order: Tables 1-5 → FORWARD, Tables 6-10 → BACKWARD
- Multiple Orders: Directional Grouping + 5F/4B rule
  Step 1: Detect multi-order (>1 order)
  Step 2: Count RIGHT (1-5) vs LEFT (6-10), start with MORE (tie=LEFT)
  Step 3: Serve all tables in that range opportunistically
  Step 4: Use 5F/4B rule for remaining tables
  Step 5: Return based on last table (1-5→BWD, 6-10→FWD)
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

# ============================================== DIRECTION DECISION ALGORITHM /////////////////

def decide_direction():
    """
    Decide initial direction based on:
    - Single order: Tables 1-5 → FORWARD, Tables 6-10 → BACKWARD
    - Multiple orders: Count RIGHT(1-5) vs LEFT(6-10), start with MORE (tie=LEFT)
    Returns: (is_forward, target_table, calc_time_us)
    """
    global current_position
    
    start_time = time.perf_counter()
    
    unserved = [t for t in order_list if not order_served[t]]
    
    if not unserved:
        return None, -1, 0
    
    # ==== SINGLE ORDER RULE ====
    if len(unserved) == 1:
        table = unserved[0]
        if 1 <= table <= 5:
            # RIGHT side (1-5) → FORWARD
            is_forward = True
            target = table
            print("📍 Single order: Table 1-5 → FORWARD (RIGHT side)")
        else:
            # LEFT side (6-10) → BACKWARD
            is_forward = False
            target = table
            print("📍 Single order: Table 6-10 → BACKWARD (LEFT side)")
        
        calc_time = (time.perf_counter() - start_time) * 1000000
        return is_forward, target, calc_time
    
    # ==== MULTIPLE ORDER RULE ====
    # Step 2: Count RIGHT (1-5) vs LEFT (6-10)
    right_tables = [t for t in unserved if 1 <= t <= 5]
    left_tables = [t for t in unserved if 6 <= t <= 10]
    
    right_count = len(right_tables)
    left_count = len(left_tables)
    
    print(f"📊 RIGHT (1-5): {right_count} orders, LEFT (6-10): {left_count} orders")
    
    # Choose side with MORE orders (tie = LEFT)
    if right_count > left_count:
        is_forward = True
        target = max(right_tables)  # Farthest in forward direction
        print("📍 Multi-order: RIGHT has more → FORWARD first")
    elif left_count > right_count:
        is_forward = False
        target = min(left_tables)  # Farthest in backward direction (closest to T10)
        print("📍 Multi-order: LEFT has more → BACKWARD first")
    else:
        # Equal: default to LEFT (backward)
        if left_tables:
            is_forward = False
            target = min(left_tables)
            print("📍 Multi-order: EQUAL → Default BACKWARD (LEFT)")
        else:
            is_forward = True
            target = max(right_tables)
            print("📍 Multi-order: No LEFT orders → FORWARD")
    
    calc_time = (time.perf_counter() - start_time) * 1000000
    return is_forward, target, calc_time


def decide_next_direction():
    """
    5F/4B Rule: For remaining tables after first side
    - Forward can reach +5 tables
    - Backward can reach +4 tables
    Returns: (is_forward, target_table, calc_time_us)
    """
    global current_position
    
    start_time = time.perf_counter()
    
    unserved = [t for t in order_list if not order_served[t]]
    
    if not unserved:
        return None, -1, 0
    
    # Single remaining → use simple rule
    if len(unserved) == 1:
        table = unserved[0]
        is_forward = 1 <= table <= 5
        calc_time = (time.perf_counter() - start_time) * 1000000
        return is_forward, table, calc_time
    
    # 5F/4B Rule: Calculate steps to each unserved table
    best_forward_table = -1
    best_forward_steps = 999
    best_backward_table = -1
    best_backward_steps = 999
    
    for table in unserved:
        # Forward steps (max 5)
        if table > current_position:
            forward_steps = table - current_position
        else:
            forward_steps = (10 - current_position) + table  # Wrap around
        
        # Backward steps (max 4)
        if table < current_position:
            backward_steps = current_position - table
        else:
            backward_steps = current_position + (10 - table)  # Wrap around
        
        # Check if within reachability
        if forward_steps <= 5 and forward_steps < best_forward_steps:
            best_forward_steps = forward_steps
            best_forward_table = table
        
        if backward_steps <= 4 and backward_steps < best_backward_steps:
            best_backward_steps = backward_steps
            best_backward_table = table
    
    print(f"📊 5F/4B from T{current_position}: Fwd→T{best_forward_table}({best_forward_steps} steps), Bwd→T{best_backward_table}({best_backward_steps} steps)")
    
    # Choose direction: prefer shorter path, then backward
    if best_backward_table != -1 and (best_forward_table == -1 or best_backward_steps <= best_forward_steps):
        is_forward = False
        target = best_backward_table
    elif best_forward_table != -1:
        is_forward = True
        target = best_forward_table
    else:
        # Fallback: use simple rule
        table = unserved[0]
        is_forward = 1 <= table <= 5
        target = table
    
    calc_time = (time.perf_counter() - start_time) * 1000000
    return is_forward, target, calc_time

# ============================================== SIMULATION /////////////////

def simulate_navigation():
    """
    Simulate Directional Grouping + 5F/4B navigation with opportunistic serving
    """
    global current_position, served_count, order_served
    
    if not order_list:
        print("❌ No orders to process!")
        return
    
    print("\n" + "=" * 60)
    print("🚀 STARTING DIRECTIONAL GROUPING + 5F/4B NAVIGATION")
    print("=" * 60)
    print("Strategy:")
    print("  SINGLE: Tables 1-5 → FORWARD, Tables 6-10 → BACKWARD")
    print("  MULTI: Count sides, start with MORE, then use 5F/4B rule")
    print("=" * 60)
    
    # Reset state
    current_position = 0
    served_count = 0
    order_served = [False] * 11
    total_markers = 0
    cumulative_counter = 0
    is_first_decision = True
    
    # Save original order list (since we'll remove items during serving)
    original_order_count = len(order_list)
    
    print(f"\n📍 Starting at: Staff Station")
    print()
    
    start_time = time.perf_counter()
    
    # Main navigation loop
    while len(order_list) > 0:
        # Direction decision
        if is_first_decision:
            is_forward, target_table, calc_time = decide_direction()
        else:
            # Step 4: Use 5F/4B rule for remaining
            is_forward, target_table, calc_time = decide_next_direction()
        
        if target_table == -1:
            break  # All served
        
        distance = DISTANCE_MATRIX[current_position][target_table]
        
        print(f"🎯 Target: Table {target_table} ({distance} markers away)")
        print(f"⚡ Calculation time: {calc_time:.1f} μs")
        
        # Determine direction
        moving_forward = is_forward
        direction = "FORWARD (RIGHT side)" if moving_forward else "BACKWARD (LEFT side)"
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
        
            # Mark first decision as complete after forward movement
            is_first_decision = False
        
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
            
            # Mark first decision as complete after backward movement
            is_first_decision = False
    
    # ==== STEP 5: Return based on last table position ====
    print(f"\n🎉 All orders served!")
    
    if 1 <= current_position <= 5:
        # Last table in RIGHT side (1-5) → return BACKWARD
        print(f"🔙 Last table T{current_position} is RIGHT (1-5) → Returning BACKWARD")
        moving_forward = False
    else:
        # Last table in LEFT side (6-10) → return FORWARD
        print(f"🔙 Last table T{current_position} is LEFT (6-10) → Returning FORWARD")
        moving_forward = True
    
    # Simulate return journey
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
    print("   Directional Grouping + 5F/4B Algorithm")
    print("=" * 60)
    print()
    print("🗺️  Restaurant Layout:")
    print("   Staff ←→ T1 → T2 → T3 → T4 → T5 → T6 → T7 → T8 → T9 → T10 ←→ Staff")
    print("   RIGHT side (1-5): FORWARD | LEFT side (6-10): BACKWARD")
    print()
    print("📋 Navigation Algorithm:")
    print("   SINGLE ORDER: Tables 1-5 → FORWARD, Tables 6-10 → BACKWARD")
    print("   MULTIPLE ORDERS:")
    print("     Step 1: Detect multi-order (>1 order)")
    print("     Step 2: Count RIGHT(1-5) vs LEFT(6-10), start with MORE (tie=LEFT)")
    print("     Step 3: Serve all tables in that range opportunistically")
    print("     Step 4: Use 5F/4B rule for remaining tables")
    print("     Step 5: Return based on last table (1-5→BWD, 6-10→FWD)")
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
    print("1. Single Order - RIGHT side (FORWARD):")
    print("   add 3")
    print("   Expected: Staff → FORWARD → T3 → BACKWARD → Staff")
    print()
    print("2. Single Order - LEFT side (BACKWARD):")
    print("   add 8")
    print("   Expected: Staff → BACKWARD → T8 → FORWARD → Staff")
    print()
    print("3. Multi-Order - More on RIGHT (example from user):")
    print("   add 1,2,3,9,8")
    print("   Expected: RIGHT(3) > LEFT(2) → FORWARD first")
    print("   Staff → T1 → T2 → T3 → [5F/4B] → T9 → T8 → FORWARD → Staff")
    print()
    print("4. Multi-Order - More on LEFT:")
    print("   add 1,7,8,9,10")
    print("   Expected: LEFT(4) > RIGHT(1) → BACKWARD first")
    print("   Staff → T10 → T9 → T8 → T7 → [5F/4B] → T1 → BACKWARD → Staff")
    print()
    print("5. Multi-Order - Equal (default to LEFT):")
    print("   add 1,2,9,10")
    print("   Expected: LEFT(2) = RIGHT(2) → BACKWARD (default LEFT)")
    print()
    print("6. All Tables:")
    print("   add 1,2,3,4,5,6,7,8,9,10")
    print("   Watch algorithm handle all tables!")
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

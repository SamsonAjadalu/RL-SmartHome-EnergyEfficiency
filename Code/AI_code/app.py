from flask import Flask, request, jsonify, render_template
import numpy as np
import random
from datetime import datetime
import pytz
from flask_limiter import Limiter
import logging
from flask_cors import CORS
import os
import json

app = Flask(__name__)

@app.route('/')
def home():
    return render_template('index.html')

CORS(app)

# Rate limiter to avoid abuse of the API
limiter = Limiter(
    app,
    # Limit based on client's IP address
    default_limits=["100 per minute"]  # Set a limit of 100 requests per minute per client
)

# Q-Learning parameters
learning_rate = 0.1  # Learning rate
discount_factor = 0.9  # Discount factor
max_memory = 100000  # Max number of data points to store
q_table_updates = []  # Store updates (state, action, reward)
q_table_file = 'q_table_updates.json'
exploration_rate = 1.0  # Initial exploration rate
min_exploration_rate = 0.01  # Minimum exploration rate
exploration_decay = 0.995  # Decay factor for exploration rate

# Define the states: (temperature, motion, time of day, light state, fan state)
states = [
    (temp, motion, time, light, fan)
    for temp in range(3)  # 0=Low, 1=Normal, 2=High temperature
    for motion in range(2)  # 0=No motion, 1=Motion detected
    for time in range(24)  # 24-hour time
    for light in range(2)  # 0=Light off, 1=Light on
    for fan in range(3)  # 0=Fan off, 1=Fan low, 2=Fan high
]

# Define the possible actions for the AI
actions = ['turn_off_fan', 'turn_on_fan_low', 'turn_on_fan_high', 'turn_off_light', 'turn_on_light']

# Initialize Q-table with random values
q_table = np.random.rand(len(states), len(actions))

# Save and load Q-table
def save_q_table():
    np.save('q_table.npy', q_table)

def load_q_table():
    global q_table
    if os.path.exists('q_table.npy'):
        q_table = np.load('q_table.npy')
    else:
        q_table = np.random.rand(len(states), len(actions))

load_q_table()

# Load updates from a file
def load_updates_from_file():
    if os.path.exists(q_table_file):
        try:
            with open(q_table_file, 'r') as file:
                data = json.load(file)
                if isinstance(data, list):
                    q_table_updates.clear()
                    for entry in data:
                        if all(key in entry for key in ['state', 'action', 'reward', 'next_state']):
                            q_table_updates.append(entry)
        except json.JSONDecodeError as e:
            logging.error(f"Error reading JSON file: {e}")

load_updates_from_file()

# Map actions and states to indices
action_index_map = {action: i for i, action in enumerate(actions)}
state_index_map = {state: i for i, state in enumerate(states)}

# Track the current state and last action
current_state = None
previous_action = None

def select_action(state):
    global exploration_rate
    state_index = state_index_map[state]

    if random.uniform(0, 1) < exploration_rate:
        # Explore: choose a random action
        action_index = random.choice(range(len(actions)))
        logging.info(f"Exploring: {actions[action_index]}")
    else:
        # Exploit: choose the best action based on the Q-table
        max_value = np.max(q_table[state_index, :])
        best_actions = np.flatnonzero(q_table[state_index, :] == max_value)
        action_index = np.random.choice(best_actions)
        logging.info(f"Exploiting: {actions[action_index]}")

    exploration_rate = max(min_exploration_rate, exploration_rate * exploration_decay)  # Decay exploration rate
    return actions[action_index]

# Update Q-table
def update_q_table(state_index, action_index, reward, next_state_index):
    q_table[state_index, action_index] = (1 - learning_rate) * q_table[state_index, action_index] + \
                                         learning_rate * (reward + discount_factor * np.max(q_table[next_state_index, :]))
    save_q_table()

    q_table_updates.append({
        "state": list(states[state_index]),
        "action": actions[action_index],
        "reward": reward,
        "next_state": list(states[next_state_index])
    })

    if len(q_table_updates) > max_memory:
        q_table_updates.pop(0)

    save_q_table_updates()

# Save Q-table updates to file
def save_q_table_updates():
    try:
        with open(q_table_file, 'w') as file:
            json.dump(q_table_updates, file, indent=4)
        logging.info("Q-table updates saved.")
    except IOError as e:
        logging.error(f"Failed to save updates: {e}")

@app.route('/api/reset_q_table', methods=['POST'])
def reset_q_table():
    global q_table_updates, q_table
    q_table = np.random.rand(len(states), len(actions))
    q_table_updates = []
    save_q_table_updates()
    save_q_table()
    return jsonify({"message": "Q-table reset complete"})

# Handle environment data
@app.route('/api/environment', methods=['POST'])
def handle_environment_data():
    global current_state
    data = request.get_json()

    try:
        temperature = data['temperature']
        motion = data['motion']
        light_state = data['lightState']
        fan_speed = data['fanSpeed']
        time_of_day = get_current_time_of_day()

        if not (0 <= temperature <= 50):
            raise ValueError(f"Invalid temperature: {temperature}")
        if motion not in [0, 1]:
            raise ValueError(f"Invalid motion state: {motion}")
        if fan_speed not in [0, 125, 255]:
            raise ValueError(f"Invalid fan speed: {fan_speed}")

        temp_state = 0 if temperature < 22 else (1 if temperature <= 28 else 2)
        motion_state = 1 if motion else 0
        fan_state = 0 if fan_speed == 0 else (1 if fan_speed == 125 else 2)

        current_state = (temp_state, motion_state, time_of_day, light_state, fan_state)
        return jsonify({"message": "Environment data received"})
    except (ValueError, KeyError) as e:
        logging.error(f"Error processing environment data: {e}")
        return jsonify({"error": str(e)}), 400

def get_current_time_of_day(timezone="Africa/Lagos"):
    """Get the time of day based on the current hour."""
    utc_now = datetime.utcnow()
    local_timezone = pytz.timezone(timezone)
    local_time = utc_now.astimezone(local_timezone)

    current_hour = local_time.hour
    return current_hour

# Get AI action
@app.route('/api/get_action', methods=['GET'])
def get_ai_action():
    global current_state

    if current_state is not None:
        action = select_action(current_state)
        logging.info(f"Action: {action} for state {current_state}")
        return jsonify({"action": action})
    else:
        return jsonify({"error": "No environment data received yet"}), 400

# Calculate reward
@app.route('/api/calculate_reward', methods=['POST'])
def calculate_reward():
    global current_state
    data = request.get_json()

    curr_temp = data['currTemp']
    curr_motion = data['currMotion']
    curr_light_state = data['currLightState']
    curr_fan_speed = data['currFanSpeed']
    action = data['action']
    time_of_day = get_current_time_of_day()

    temp_state = 0 if curr_temp < 22 else (1 if curr_temp <= 28 else 2)
    motion_state = 1 if curr_motion else 0
    fan_state = 0 if curr_fan_speed == 0 else (1 if curr_fan_speed == 125 else 2)
    light_state = 1 if curr_light_state else 0

    next_state_index = state_index_map[(temp_state, motion_state, time_of_day, light_state, fan_state)]
    action_index = action_index_map[action]

    reward = calculate_reward_logic(current_state[0], curr_temp, current_state[1], curr_motion,
                                    current_state[3], curr_light_state, current_state[4], curr_fan_speed,
                                    action, time_of_day)

    update_q_table(state_index_map[current_state], action_index, reward, next_state_index)
    current_state = (temp_state, motion_state, time_of_day, light_state, fan_state)

    return jsonify({"message": "Reward calculated", "reward": reward})

# Logic to calculate reward
def calculate_reward_logic(prev_temp, curr_temp, prev_motion, curr_motion, prev_light_state, curr_light_state, prev_fan_speed,
                           curr_fan_speed, action, time_of_day):
    reward = 0

    # Map the actions to the expected fan and light states
    action_to_fan_state = {
        'turn_off_fan': 0,
        'turn_on_fan_low': 1,
        'turn_on_fan_high': 2
    }

    action_to_light_state = {
        'turn_off_light': 0,
        'turn_on_light': 1
    }

    # Prevent turning on fan or light when no motion is detected
    if curr_motion == 0:  # No motion
        if action == "turn_on_light" or action == "turn_on_fan_low" or action == "turn_on_fan_high":
            return -10  # Strong penalty for turning on devices when no motion is detected

    # Ensure actions are meaningful (no reward for redundant actions)
    if action in action_to_fan_state and action_to_fan_state[action] == prev_fan_speed:
        reward -= 2  # Small penalty for redundant actions
    if action in action_to_light_state and action_to_light_state[action] == prev_light_state:
        reward -= 2  # Small penalty for redundant actions

    # Handle motion and comfort
    if curr_motion == 1:  # Motion detected
        if action == "turn_on_light":
            reward += 10  # Reward for turning on light when motion is detected (comfort)
        elif action == "turn_off_light":
            reward -= 5  # Penalty for turning off light during motion (discomfort)

        if action == "turn_on_fan_low":
            reward += 8  # Reward for using low fan when motion is detected (comfort & efficiency)
        elif action == "turn_on_fan_high":
            reward += 5  # Moderate reward for using high fan when needed
        elif action == "turn_off_fan":
            reward -= 8  # Penalty for turning off fan during motion (discomfort)

    else:  # No motion detected
        if action == "turn_off_light" or action == "turn_off_fan":
            reward += 10  # Strong reward for turning off devices (high energy savings)
        elif action == "turn_on_light" or action == "turn_on_fan_high":
            reward -= 10  # Strong penalty for turning on devices with no motion (wasting energy)
        elif action == "turn_on_fan_low":
            reward -= 7  # Moderate penalty for turning on low fan without motion

    # Add time-based penalties or rewards for light usage
    reward += time_based_light_control(action, time_of_day, curr_motion)

    # Temperature-based rewards for fan usage
    reward += temperature_based_fan_control(prev_temp, curr_temp, action)

    # Energy-saving rewards for light and fan state changes
    reward += energy_saving_rewards(prev_light_state, curr_light_state, prev_fan_speed, curr_fan_speed, action)

    return reward

def time_based_light_control(action, time_of_day, curr_motion):
    """Calculate reward or penalty based on the time of day and motion detection for light control."""
    time_rewards = {
        range(6, 9): -3,  # Morning (6 AM - 9 AM) - Penalty for turning on lights
        range(9, 12): -2,  # Mid-morning (9 AM - 12 PM) - Mild penalty for turning on lights
        range(12, 15): -1,  # Early afternoon (12 PM - 3 PM) - Small penalty for turning on lights
        range(15, 18): 0,  # Late afternoon (3 PM - 6 PM) - No penalty
        range(18, 21): 5,  # Evening (6 PM - 9 PM) - Reward for turning on lights
        range(21, 24): 3,  # Night (9 PM - 12 AM) - Small reward for turning on lights
        range(0, 6): 3  # Night (12 AM - 6 AM) - Small reward for turning on lights
    }

    reward = 0

    # If the action is to turn on the light, apply the time-based penalty/reward
    if action == "turn_on_light":
        for time_range, time_reward in time_rewards.items():
            if time_of_day in time_range:
                reward = time_reward
                break

    # Penalty if turning off light in the evening when motion is detected
    if action == "turn_off_light" and 18 <= time_of_day < 21 and curr_motion == 1:
        reward -= 5  # Penalty for turning off lights during motion in the evening

    return reward

def temperature_based_fan_control(prev_temp, curr_temp, action):
    """Calculate reward or penalty based on temperature and fan usage."""
    reward = 0
    if curr_temp > 30:  # High temperature
        if action == "turn_on_fan_high":
            reward += 10  # Strong reward for using high fan to cool down
        elif action == "turn_on_fan_low":
            reward += 7  # Strong reward for using low fan (efficiency)
        elif action == "turn_off_fan":
            reward -= 10  # Strong penalty for turning off fan when it's hot
    elif 22 <= curr_temp <= 30:  # Comfortable temperature
        if action == "turn_on_fan_high":
            reward -= 3  # Small penalty for turning on high fan when it's comfortable
        elif action == "turn_on_fan_low":
            reward += 4  # Reward for maintaining comfort with low fan
        elif action == "turn_off_fan":
            reward += 5  # Reward for turning off fan in comfortable temperature
    elif curr_temp < 22:  # Low temperature
        if action == "turn_on_fan_high":
            reward -= 8  # Penalty for turning on high fan when it's cold
        elif action == "turn_on_fan_low":
            reward -= 5  # Small penalty for using low fan in cold temperature
        elif action == "turn_off_fan":
            reward += 7  # Reward for turning off fan when it's cold

    return reward

def energy_saving_rewards(prev_light_state, curr_light_state, prev_fan_speed, curr_fan_speed, action):
    """Calculate reward for energy-saving behaviors."""
    reward = 0
    # Energy-saving reward for light state change
    if prev_light_state == 0 and curr_light_state == 1:
        if action == "turn_on_light":
            reward += 1  # Small reward for turning on light
    elif prev_light_state == 1 and curr_light_state == 0:
        if action == "turn_off_light":
            reward += 6  # Reward for turning off light (energy saving)

    # Energy-saving reward for fan state change
    if prev_fan_speed == 0 and curr_fan_speed == 1:  # Low fan speed
        if action == "turn_on_fan_low":
            reward += 5  # Reward for energy-efficient cooling
    elif prev_fan_speed == 0 and curr_fan_speed == 2:  # High fan speed
        if action == "turn_on_fan_high":
            reward += 3  # Small reward for using high fan
    elif prev_fan_speed > 0 and curr_fan_speed == 0:
        if action == "turn_off_fan":
            reward += 7  # Reward for turning off the fan (energy saving)

    return reward

# Manual override handling
override_tracker = {}

@app.route('/api/manual_override', methods=['POST'])
def handle_manual_override():
    global current_state, previous_action
    data = request.get_json()

    if data.get('manualOverride'):
        temp_state = 0 if data['temperature'] < 22 else (1 if 22 <= data['temperature'] <= 28 else 2)
        motion_state = 1 if data['motion'] else 0
        fan_state = 0 if data['fanSpeed'] == 0 else (1 if data['fanSpeed'] == 125 else 2)
        time_of_day = get_current_time_of_day()
        new_current_state = (temp_state, motion_state, time_of_day, data['lightState'], fan_state)

        if current_state:
            last_state = current_state
            last_action = choose_action(last_state)
            # Determine the penalty for the overridden action

            penalty = -20  # General penalty

            # Update the Q-table using the last state and the new current state after the manual override
            last_state_idx = state_to_index[last_state]
            new_current_state_idx = state_to_index[new_current_state]
            update_q_table(last_state_idx, action_to_index[last_action], penalty, new_current_state_idx)

        logging.info(f"Manual override activated. Action overridden: {last_action}")

    return jsonify({"message": "Manual override acknowledged, AI dynamically penalized."})



if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)

import pandas as pd
import random
import datetime
import numpy as np

# Parameters
num_systems = 5 # Number of Soil Moisture Monitoring Systems
num_days = 365  # Data over a year
readings_per_day = 2  # Two readings per day (4 AM and 12 PM)
reading_times = [4, 12]  # 4 AM and 12 PM
base_location = (-33.0, 121.0)  # South Western Australia (approximate coordinates)
location_variation = 0.01  # 1 km variation

# Sensor Depths
sensor_depths = [10, 20, 30]

# Helper functions to generate mock data
def generate_random_location(base_lat, base_long, variation):
    """Generate GPS coordinates within a 1km range of a base location."""
    latitude = base_lat + random.uniform(-variation, variation)
    longitude = base_long + random.uniform(-variation, variation)
    return f"{latitude:.6f}, {longitude:.6f}"

def generate_soil_moisture(day_of_year, depth):
    """Generate a cyclic soil moisture value based on the day of the year and sensor depth."""
    # Sinusoidal variation to simulate seasonal changes
    seasonal_variation = 15 + 10 * np.sin(2 * np.pi * day_of_year / 365)
    # Add noise and depth effect
    depth_effect = depth / 10  # Deeper sensors may retain more moisture
    moisture = seasonal_variation + random.uniform(-2, 2) + depth_effect
    return round(moisture, 2)

# Generate mock data
data = []
gateway_id = "Gateway001"
gateway_data = []

for system_id in range(1, num_systems + 1):
    soil_moisture_system_id = f"SMS{system_id:03d}"
    location = generate_random_location(base_location[0], base_location[1], location_variation)
    base_time = datetime.datetime.now() - datetime.timedelta(days=num_days)
    
    for day in range(num_days):
        for reading_time in reading_times:
            timestamp = base_time + datetime.timedelta(days=day, hours=reading_time)
            day_of_year = timestamp.timetuple().tm_yday
            
            for depth in sensor_depths:
                sensor_id = f"Sensor{depth}cm"
                sensor_data = {
                    'SoilMoistureSystemID': soil_moisture_system_id,
                    'SensorID': sensor_id,
                    'Timestamp': timestamp,
                    'Location': location,
                    'SoilMoisture(%)': generate_soil_moisture(day_of_year, depth)
                }
                data.append(sensor_data)
            
            # Gateway data (transmission occurs a few seconds after sensor reading)
            gateway_timestamp = timestamp + datetime.timedelta(seconds=random.randint(1, 10))
            gateway_data.append({
                'GatewayID': gateway_id,
                'SoilMoistureSystemID': soil_moisture_system_id,
                'GatewayTimestamp': gateway_timestamp,
                'DataTransmitted': True
            })

# Convert data to DataFrames
df = pd.DataFrame(data)
df_gateway = pd.DataFrame(gateway_data)

# Combine both DataFrames for easier analysis
combined_df = pd.merge(df, df_gateway, on='SoilMoistureSystemID', how='left')

# Save the data to CSV files for import into Power BI or other tools
df.to_csv('mock_soil_moisture_data.csv', index=False)
df_gateway.to_csv('mock_gateway_data.csv', index=False)
combined_df.to_csv('mock_combined_data.csv', index=False)

print("Mock data generated and saved to 'mock_soil_moisture_data.csv', 'mock_gateway_data.csv', and 'mock_combined_data.csv'")

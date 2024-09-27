''' 
Lambda function is responsible for:
- Extracting and transforming message delivered to AWS IoT Core topic from the Internet Gateway Master Node
- Create item into DynamoDb Table called `SoilMoistureData` with the transformed message
- Error handling with appropriate messages for:
    Invalid paylod structure in AWS IoT Core Topic
    DynamoDB Write Errors
    Missing/Incorrect Data
    Catch all (Unexpected exceptions)

Developed by Asad Maza
v1.0
'''

# Import necessary libraries
import json
import boto3
import logging

# Start DynamoDB and Logger
dynamodb = boto3.resource('dynamodb')
table = dynamodb.Table('SoilMoistureData')
logger = logging.getLogger()
logger.setLevel(logging.INFO)

def lambda_handler(event, context):
    try:
        # Log event for debugging
        logger.info(f"Received event: {json.dumps(event)}")

        # Use event as message
        message = event
        logger.info(f"Parsed Message: {message}")
        
        # Validate message is a list
        if not isinstance(message, list):
            logger.error("Invalid message format: Expected a list.")
            return {
                'statusCode': 400,
                'body': json.dumps('Error: Invalid message format. Expected a list.')
            }
        
        # Validate message length (Expected 16 fields: 5 sensor data, 6 alternating sensor depth/moisture, 5 gateway data)
        if len(message) != 16:
            logger.error(f"Malformed message: expected 16 fields, got {len(message)} fields.")
            return {
                'statusCode': 400,
                'body': json.dumps(f'Error: Malformed message. Expected exactly 16 fields, got {len(message)}.')
            }

        # Parse sensor board data (first 5 values)
        sensor_board_id = message[0]
        sensor_board_date = message[1]
        sensor_board_time = message[2]
        sensor_board_coordinates = message[3]  # Currently not split coordinates
        sensor_board_battery = message[4]
        
        # Parse sensor readings (alternating sensor depths and moisture levels)
        depth_values = message[5:10:2]  # Extracts 1st, 3rd, and 5th numbers as depths (sensor depths)
        soil_moisture_levels = message[6:11:2]  # Extracts 2nd, 4th, and 6th numbers as moisture levels
        
        # Map depths to their corresponding moisture levels
        moisture_data = {f"{depth}cm": f"{moisture_level}%" for depth, moisture_level in zip(depth_values, soil_moisture_levels)}
        
        # Parse gateway board data (last 5 values)
        gateway_id = message[11]  # Gateway ID
        gateway_date = message[12]
        gateway_time = message[13]
        gateway_coordinates = message[14]  # Do not split coordinates
        gateway_battery = message[15]  # Gateway Battery

        # Combine sensor board date and time for sensor timestamp
        sensor_timestamp = f"{sensor_board_date}T{sensor_board_time}"
        
        # Combine gateway date and time for gateway timestamp
        gateway_timestamp = f"{gateway_date}T{gateway_time}"
        
        # Prepare item to insert into DynamoDB
        item = {
            'gateway_id': str(gateway_id),  # Ensure this is stored as a string
            'sensor_board_id_timestamp': f"{sensor_board_id}_{sensor_timestamp}",
            'gateway_time': gateway_timestamp,
            'gateway_battery': gateway_battery,
            'gateway_location': gateway_coordinates,  # Store as a single string
            'sensor_board_location': sensor_board_coordinates,  # Store as a single string
            'sensor_board_battery': sensor_board_battery,
            'moisture_data': moisture_data
        }

        # Log item to be inserted (for debugging)
        logger.info(f"Inserting item into DynamoDB: {json.dumps(item)}")

        # Attempt to insert item into DynamoDB
        try:
            table.put_item(Item=item)
        except Exception as e:
            logger.error(f"DynamoDB put_item error: {e}")
            return {
                'statusCode': 500,
                'body': json.dumps(f'Error: Unable to insert data into DynamoDB. {str(e)}')
            }
        
        # Success response
        return {
            'statusCode': 200,
            'body': json.dumps('Data successfully saved to DynamoDB!')
        }

    except json.JSONDecodeError as e:
        # Handle JSON parsing error
        logger.error(f"JSON decoding failed: {e}")
        return {
                'statusCode': 400,
                'body': json.dumps('Error: Invalid JSON format.')
        }
    except Exception as e:
        # Catch all for unexpected errors
        logger.error(f"Unexpected error: {e}")
        return {
            'statusCode': 500,
            'body': json.dumps(f'Error: {str(e)}')
        }
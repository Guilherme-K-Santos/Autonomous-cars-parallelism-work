import random
import time

# Generates random numbers.
def getRandomNumbers(min, max):
    return random.randint(min, max)

# Gets a command from the control center and do it. 
def atuador(atuador_id, activity_level):
    print('ID: ', atuador_id, 'Activity Level: ', activity_level)
    pass

# Generates random data for use in tha entire algorithm
def sensor(num_sensors):
    print('a', num_sensors)
    sensor_data = {}
    for i in range(num_sensors):
        # Simulates timeout to get data from sensor
        time.sleep(getRandomNumbers(1, 5))

        # Generates a random data and stores into a dict (idk but it maybe seems useful in the future)
        sensor_data[i] = getRandomNumbers(1, 1000)
        
    # Sends data generated to control center
    return sensor_data
        
# Receives data from a sensor and transform in command for "atuador" usage.
def controlCenter(num_sensors, num_atuadores):
    raw_data = sensor(num_sensors)

    # This `for` for now is only for log data purposes. All data will be log after sensor process.
    # I suppose that we'll need to call atuador() here.
    for sen in raw_data:
        print('sensor: ', sen, 'sent: ', raw_data[sen])
        # Do something with the raw received data
        # 
        # 

        # Defines an `atuador` (based on the calculation below) and activity level (random 1 - 100 number) 
        atuador(raw_data[sen] % num_atuadores, getRandomNumbers(0, 100))

    print('EXIT')

controlCenter(int(input()), int(input()))

# KEEP IN MIND: it is normal code delays because we have not prints in sensor. If you
# need a better understanding put some prints there.
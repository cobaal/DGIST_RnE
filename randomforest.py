import serial
import numpy as np             
from numpy import array
import pandas as pd
from sklearn.model_selection import train_test_split
from sklearn.ensemble import RandomForestClassifier
from sklearn import metrics
from sklearn.metrics import accuracy_score


train_x = []
train_y = []
with open("./sihoon_data.txt", "r") as filestream:
    while True:
        cline = filestream.readline().split(",")
        if not cline or len(cline) < 2 :
          break
        cline[0] = float(cline[0])
        cline[1] = int(cline[1])
        cline[2] = int(cline[2])
        cline[3] = int(cline[3])
        cline[4] = int(cline[4])
   
        cline = tuple(cline)
        train_x.append(cline)
   

df = pd.DataFrame(train_x)
df.columns = ['H_temperature', 'Humidity',
            'C_temperature',
            'Heart_rate',
            'target_temperature']

x = df[['H_temperature', 'Humidity',
            'C_temperature',
            'Heart_rate']]
y = df['target_temperature']


x_train, x_test, y_train, y_test = train_test_split(x, y, test_size=0.3)
  
print(len(x_train))
print(len(x_test))
print(len(y_train))
print(len(y_test))
 


forest = RandomForestClassifier(n_estimators=100)
forest.fit(x_train, y_train)
 

y_pred = forest.predict(x_test)
print(y_pred)
print(list(y_test))
 

print(metrics.accuracy_score(y_test, y_pred))


ser = serial.Serial('COM20',9600)

while True:
    
    rx = ser.readline().decode('UTF-8').split(',')
    if len(rx) == 5:
        arr = []
        arr.append(float(rx[0]))
        arr.append(int(rx[1]))
        arr.append(int(rx[2]))
        arr.append(int(rx[3]))

        data = []
        data.append(arr)

        curTemp = int(rx[4])
        tarTemp = forest.predict(data)
        print(arr, "/ Current Temp:", curTemp , "/ Target Temp:", tarTemp[0])

        if curTemp != tarTemp[0]:
            ser.write(str(chr(tarTemp)).encode("utf-8"))

ser.close()

from flask import Flask, request, jsonify
from pymongo import MongoClient
import pymongo
from bson.binary import Binary
import numpy as np
import os
import pickle
from dotenv import load_dotenv
from sklearn.preprocessing import LabelEncoder, OneHotEncoder
import tensorflow as tf


load_dotenv()


uri = f"mongodb+srv://pragnesh-barik:{os.getenv('MONGODB_PASSWORD')}@cluster0.rkh1i.mongodb.net/?retryWrites=true&w=majority"
client = MongoClient(uri)
db = client.gait.research


app = Flask(__name__)


@app.route('/log', methods=['POST'])
def log():
    data = request.get_json()
    print(data)
    # convert data to float
    data["image"] = np.array(data["image"]).astype(float)
    data["image"] = Binary(
        pickle.dumps(data["image"], protocol=2))
    res = db.insert_one(data)
    print(res.inserted_id)
    # load data from mongo
    # y = pickle.loads(db.find_one()["image"])
    response = {'message': 'Data received successfully'}
    return jsonify(response), 200


def one_hot(labels) :
    classes = [
        "Walking" ,
        "Running" ,
        "Stair Up" ,
        "Stair Down" ,
        "Stationary" ,
    ]

    
    encodings = {}
    z = np.zeros(len(classes))
    for i, cls in enumerate(classes):
        encodings[cls] = z
        encodings[cls][i] = 1

    one_hot = []

    for train_label in labels:
        one_hot.append(encodings[train_label])

    one_hot = np.array(one_hot)
    return one_hot


@app.route("/")
def train():
    load_images = pickle.load(open("./data/images.pkl", "rb"))
    load_labels = pickle.load(open("./data/labels.pkl", "rb"))
    one_hot_labels = one_hot(load_labels)  
            

    print(load_images.shape)
    train_images = np.resize(load_images,(len(load_images), 120, 7, 1))
    print(train_images.shape)
    print(load_labels.shape)

    model = tf.keras.models.Sequential([
        tf.keras.layers.Conv2D(32, (20, 3), activation='relu', input_shape=(120, 7, 1),), # 4 * 100
        tf.keras.layers.Conv2D(64, (10, 1), activation='relu', ), # 3
        tf.keras.layers.MaxPooling2D((2, 2)), # 2
        tf.keras.layers.Flatten(),
        tf.keras.layers.Dense(32, activation='relu'),
        tf.keras.layers.Dense(5, activation='sigmoid')
    ])

    print(model.summary())

    model.compile(optimizer='adam',loss=tf.keras.losses.CategoricalCrossentropy(from_logits=True), metrics=['accuracy'])

    history = model.fit(train_images, one_hot_labels, epochs=10)

    return history.history



@app.route("/gen_dataset")
def gen_dataset():
    images = []
    labels = []
    docs = db.find()
    for doc in docs:
        image = pickle.loads(doc["image"])
        images.append(image)
        labels.append(doc["label"])
    
    images = np.array(images)
    # print(images.shape)
    labels = np.array(labels)

    pickle.dump(images, open("./data/images.pkl", "wb"))    
    pickle.dump(labels, open("./data/labels.pkl", "wb")) 

    return "success"   

if __name__ == '__main__':
    app.run(debug=True)

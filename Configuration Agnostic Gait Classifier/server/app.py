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
from sklearn.utils import shuffle


load_dotenv()


uri = f"mongodb+srv://pragnesh-barik:{os.getenv('MONGODB_PASSWORD')}@cluster0.rkh1i.mongodb.net/?retryWrites=true&w=majority"
client = MongoClient(uri)
db = client.gait.research


app = Flask(__name__)

model = tf.keras.models.load_model("data/classifier.h5")
classes = [
    "Walking",
    "Fast Walking",
    "Running",
    "Stair Up",
    "Stair Down",
    "Stationary",
]


@app.route("/log", methods=["POST"])
def log():
    data = request.get_json()
    # print(data)
    # convert data to float
    data["image"] = np.array(data["image"]).astype(float)
    for_prediction = data["image"].reshape(1, 200, 6, 1)
    data["image"] = Binary(pickle.dumps(data["image"], protocol=2))
    res = db.insert_one(data)
    print(res.inserted_id)

    x = model.predict(for_prediction)
    # data["image"] = Binary(pickle.dumps(data["image"], protocol=2))
    # res = db.insert_one(data)
    # print(res.inserted_id)
    print(classes[np.argmax(x)])
    # load data from mongo
    # y = pickle.loads(db.find_one()["image"])
    response = {
        "message": "Data received successfully",
        "prediction": classes[np.argmax(x)],
    }
    return jsonify(response), 200


def one_hot(labels):
    encodings = {}
    for i, cls in enumerate(classes):
        z = np.zeros(len(classes))
        encodings[cls] = z
        encodings[cls][i] = 1

    one_hot = []

    for train_label in labels:
        one_hot.append(encodings[train_label])

    one_hot = np.array(one_hot)
    return one_hot


@app.route("/train")
def train():
    load_images = pickle.load(open("./data/images.pkl", "rb"))
    load_labels = pickle.load(open("./data/labels.pkl", "rb"))

    print(load_labels)
    X = np.resize(load_images, (len(load_images), 200, 6, 1))
    y = one_hot(load_labels)
    X, y = shuffle(X, y, random_state=0)
    model = tf.keras.models.Sequential(
        [
            tf.keras.layers.Conv2D(
                32,
                (10, 3),
                activation="relu",
                input_shape=(200, 6, 1),
            ),  # 4 * 100
            tf.keras.layers.Conv2D(
                64,
                (5, 2),
                activation="relu",
            ),  # 3
            tf.keras.layers.Conv2D(
                128,
                (1, 2),
                activation="relu",
            ),  # 3
            tf.keras.layers.Flatten(),
            # tf.keras.layers.Dense(256, activation='relu'),
            tf.keras.layers.Dense(32, activation="relu"),
            tf.keras.layers.Dense(6, activation="softmax"),
        ]
    )

    print(model.summary())

    model.compile(
        optimizer="adam",
        loss=tf.keras.losses.CategoricalCrossentropy(),
        metrics=["accuracy"],
    )
    history = model.fit(X, y, epochs=10)
    return history.history


@app.route("/predict", methods=["POST"])
def predict():
    data = request.get_json()
    # print(data)

    # convert data to float
    data = np.array(data).astype(float)
    print(data.shape)


@app.route("/")
def hello():
    return "Hello World!"


@app.route("/gen_dataset")
def gen_dataset():
    images = []
    labels = []
    docs = db.find()
    for doc in docs:
        image = pickle.loads(doc["image"])
        image = np.resize(image, (200, 6, 1))
        images.append(image)
        labels.append(doc["label"])

    images = np.array(images)
    print(images.shape)
    labels = np.array(labels)

    pickle.dump(images, open("./data/images.pkl", "wb"))
    pickle.dump(labels, open("./data/labels.pkl", "wb"))

    return "success"


if __name__ == "__main__":
    load_model()
    # train()
    # gen_dataset()
    # print(model.summary())
    app.run(debug=True)

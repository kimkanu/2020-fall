from flask import Flask, render_template, request
import json
import hashlib

app = Flask(__name__)


@app.route('/hello_world', methods=['GET'])
def route_hello_world():
    return json.dumps({'message': 'hello world'})


@app.route('/hash', methods=['GET'])
def route_hash():
    name = request.args.get('name')
    hash = hash_sha256(name)
    return json.dumps({'result': hash})


@app.route('/collatz', methods=['POST'])
def route_collatz():
    name = request.form.get('name')
    hash_in_request = request.form.get('hash')
    hash = hash_sha256(name)
    if hash != hash_in_request:
        return json.dumps({'error': 'HASH NOT MATCHED'})

    number = request.form.get('number')
    try:
        return json.dumps({'result': collatz(int(number))})
    except ValueError:
        return json.dumps({'error': 'NUMBER NOT INTEGER'})


def hash_sha256(text):
    return hashlib.sha256(text.encode()).hexdigest()


def collatz(number):
    if number % 2 == 0:
        return int(number / 2)
    return 3 * number + 1


if __name__ == '__main__':
    app.run()

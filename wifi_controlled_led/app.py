from flask import Flask, request, jsonify, send_from_directory
from flask_cors import CORS


app = Flask(__name__)
CORS(app)

LED_STATE = 0.5

@app.route('/')
def serve_index():
    return send_from_directory('static', 'index.html')

@app.route('/LED', methods=['GET', 'POST'])
def led_state():
    global LED_STATE
    if request.method == 'POST':
        dct = request.json
        val = dct['led_state']
        if 0.0 <= val <= 1.0:
            LED_STATE = val
        return jsonify({'status': 'ok', 'led_state': LED_STATE}), 200
    elif request.method == 'GET':
        return jsonify({'status': 'ok', 'led_state': LED_STATE}), 200
    else:
        return jsonify({'status': 'error', 'msg': 'invalid request'}), 400


if __name__ == '__main__':
    app.run(host='192.168.1.67', port=5000, debug=True)
    



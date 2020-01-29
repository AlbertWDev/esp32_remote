import os
from stat import ST_MODE, ST_MTIME, ST_SIZE

from flask import Flask
from flask import jsonify, abort, request, send_from_directory
from flask_cors import CORS
app = Flask(__name__)
CORS(app)


network_wifi_storage = [
    { 'ssid': 'TEST', 'password': 'password', 'score': 10 },
    { 'ssid': 'NETWORK', 'password': 'i_am_secure', 'score': 1 }
]

@app.route('/v1/network/wifi/storage', methods=['GET'])
def get_network_wifi_storage():
    return jsonify(network_wifi_storage)

@app.route('/v1/network/wifi/storage', methods=['POST'])
def post_network_wifi_storage():
    ''' Store a new network '''

    print(request.data)
    if not request.json or 'ssid' not in request.json:
        abort(404)
    network = {
        'ssid': request.json.get('ssid'),
        'password': request.json.get('password', ''),
        'score': 0 #request.json.get('score', 0)
    }
    network_wifi_storage.append(network)
    return jsonify(network)



@app.route('/v1/network/wifi/storage/<string:ssid>', methods=['PUT'])
def post_network_wifi_storage_ssid(ssid):
    ''' Update network data or create a new one if it doesn't exist'''

    if not request.json:
        abort(404)

    ## Check if network already exists
    for network in network_wifi_storage:
        if network['ssid'] == ssid:
            ## Update its data
            network['password'] = request.json.get('password', network['password'])
            #network['score'] = request.json.get('score', network['score'])
            return jsonify(network)
    
    ## Not found, create new
    network_wifi_storage.append({
        'ssid': ssid,
        'password': request.json.get('password', ''),
        'score': 0
    })
    abort(404)

@app.route('/v1/network/wifi/storage/<string:ssid>', methods=['DELETE'])
def delete_network_wifi_storage_ssid(ssid):
    for network in network_wifi_storage:
        if network['ssid'] == ssid:
            network_wifi_storage.remove(network)
            return jsonify(network)
    abort(404)


STORAGE_BASE = os.path.join(os.path.dirname(__file__), 'storage')

@app.route('/v1/storage/', methods=['GET'])
def get_storage():
    mount_points = []
    for mount_point in os.listdir(STORAGE_BASE):
        if os.path.isdir(os.path.join(STORAGE_BASE, mount_point)):
            mount_points.append({
                'name': mount_point,
                'type': 'DIR',
                'mode': None,
                'modified': -1,
                'size': 0
            })
    return jsonify(mount_points)

@app.route('/v1/storage/<path:node>', methods=['GET'])
def get_storage_node(node):
    if node[-1] == '/':
        path = os.path.join(STORAGE_BASE, node)
        nodes = []
        try:
            for filename in os.listdir(path):
                filepath = os.path.join(path, filename)
                filestat = os.stat(filepath)
                node_info = {
                    'name': filename,
                    'mode': filestat[ST_MODE],
                    'modified': filestat[ST_MTIME],
                    'size': filestat[ST_SIZE]
                }
                if os.path.isfile(filepath):
                    node_info['type'] = 'FILE'
                    nodes.append(node_info)
                elif os.path.isdir(filepath):
                    node_info['type'] = 'DIR'
                    nodes.append(node_info)
            return jsonify(nodes)
        except FileNotFoundError:
            abort(404)
    else:
        filename = os.path.join(STORAGE_BASE, node)
        if os.path.isfile(filename):
            return send_from_directory(STORAGE_BASE, node, as_attachment=True)
        else:
            abort(404)


@app.route('/v1/storage/<path:node>', methods=['PUT'])
def put_storage_node(node):
    print(request.content_type)
    mount_point = node.split('/')[0]
    if mount_point not in ['sdcard', 'spiffs']:
        abort(404)
    
    if node[-1] == '/':
        os.makedirs(os.path.join(STORAGE_BASE, node))
    elif os.path.isdir(os.path.join(STORAGE_BASE, '/'.join(node.split('/')[:-1]))):
        with open(os.path.join(STORAGE_BASE, node), 'wb') as f:
            f.write(request.data)

    return node



if __name__ == '__main__':
    app.run()
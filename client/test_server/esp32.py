import os
import random
from stat import ST_MODE, ST_MTIME, ST_SIZE

from flask import Flask
from flask import jsonify, abort, request, send_from_directory
from flask_cors import CORS
app = Flask(__name__)
CORS(app)

@app.route('/v1/system/info', methods=['GET'])
def get_system_info():
    return jsonify({
        'chip': {
            'model': 'esp32',
            'revision': 1,
            'emb_flash': False,
            'wifi_bgn': True,
            'bt': True,
            'ble': True
        },
        'idf': 'v0.0-test',
        'hostname': 'test_server',
        'mac': 'FF:FF:FF:00:00:00'
    })

@app.route('/v1/system/ram', methods=['GET'])
def get_system_ram():
    return jsonify({
        'heap_total': 293808,
        'heap_free': 190000 - random.randint(100, 150000)
    })


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


system_partitions = [
    {
        'label': 'nvs',
        'type': 'DATA',
        'subtype': 'NVS',
        'address': 0x9000,
        'size': 0x4000,
        'encrypted': True
    },
    {
        'label': 'otadata',
        'type': 'DATA',
        'subtype': 'OTA',
        'address': 0xd000,
        'size': 0x2000,
        'encrypted': False
    },
    {
        'label': 'phy_init',
        'type': 'DATA',
        'subtype': 'PHY',
        'address': 0xf000,
        'size': 0x1000,
        'encrypted': False
    },
    {
        'label': 'ota_0',
        'type': 'APP',
        'subtype': 'OTA_0',
        'address': 0x10000,
        'size': 0x100000,
        'encrypted': False,
        'boot': True,
        'running': True
    },
    {
        'label': 'ota_1',
        'type': 'APP',
        'subtype': 'OTA_1',
        'address': 0x110000,
        'size': 0x100000,
        'encrypted': False
    },
    {
        'label': 'storage',
        'type': 'DATA',
        'subtype': 'SPIFFS',
        'address': 0x210000,
        'size': 0x1f0000,
        'encrypted': False
    }
]

@app.route('/v1/system/partitions', methods=['GET'])
def get_system_partitions():
    return jsonify(system_partitions)

@app.route('/v1/system/partitions/<string:label>', methods=['GET'])
def get_system_partitions_label(label):
    ## Check if partition exists
    for partition in system_partitions:
        print(partition['label'])
        if partition['label'] == label:
            return jsonify(partition)
    
    ## Not found
    abort(404)

@app.route('/v1/system/partitions', methods=['PUT'])
def put_system_partitions():
    partition = next((p for p in system_partitions if p['type'] == 'APP' and 'OTA' in p['subtype'] and not p.get('boot', False)), None)
    if partition is None:
        abort(404)

    with open(os.path.join('.', f"{partition['label']}.bin"), 'wb') as f:
        f.write(request.data)

    return jsonify(partition)

@app.route('/v1/system/partitions/<string:label>', methods=['PUT'])
def put_system_partitions_label(label):
    partition = next((p for p in system.partitions if p['label'] == label), None)
    if partition is None:
        abort(404)

    with open(os.path.join('.', f"{partition['label']}.bin"), 'wb') as f:
        f.write(request.data)

    return jsonify(partition)

@app.route('/v1/system/partitions/<string:label>/boot', methods=['PUT'])
def put_system_partitions_label_boot(label):
    ## Check if partition exists
    for partition in system_partitions:
        if partition['label'] == label:
            if partition['type'] != 'APP':
                return abort(400)
            
            for p in system_partitions:
                p['boot'] = False
            partition['boot'] = True

            return "Boot set"
    
    ## Not found
    abort(404)

@app.route('/v1/system/partitions/<string:label>', methods=['DELETE'])
def delete_system_partitions_label(label):
    ## Check if partition exists
    for partition in system_partitions:
        if partition['label'] == label:
            if 'running' in partition and partition['running'] == True:
                return abort(400)
            
            if partition['subtype'] == 'SPIFFS':
                pass # Clear storage

            return ""
    
    ## Not found
    abort(404)


if __name__ == '__main__':
    app.run()
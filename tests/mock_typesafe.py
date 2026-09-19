"""Deterministic local fixture for the TypeSafe HTTP contract, never calls AI."""
import json
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer


class Handler(BaseHTTPRequestHandler):
    def log_message(self, *_):
        pass

    def do_POST(self):
        payload = json.loads(self.rfile.read(int(self.headers['Content-Length'])))
        assert self.path == '/v1/systemone'
        assert self.headers['Authorization'] == 'Bearer test-key'
        assert payload['questions']['matches']['type'] == 'noul'
        value = payload['state']['value']
        prompt = payload['questions']['matches']['instructions']['condition']
        status = 200
        body = {'answers': {'matches': {'type': 'noul', 'noul': 0.9 if value == prompt else 0.1}}}
        if value.startswith('http:'):
            status = int(value[5:])
        elif value == 'malformed':
            body = {'answers': {}}
        elif value == 'wrong-type':
            body['answers']['matches']['noul'] = '0.9'
        elif value == 'out-of-range':
            body['answers']['matches']['noul'] = 2
        elif value == 'boundary':
            body['answers']['matches']['noul'] = 0.5
        elif value == 'slow':
            time.sleep(2)
        response = json.dumps(body).encode()
        try:
            self.send_response(status)
            self.send_header('Content-Type', 'application/json')
            self.send_header('Content-Length', str(len(response)))
            self.end_headers()
            self.wfile.write(response)
        except BrokenPipeError:
            pass


ThreadingHTTPServer(('0.0.0.0', 8080), Handler).serve_forever()

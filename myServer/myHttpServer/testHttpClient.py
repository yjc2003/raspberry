'''
import http.client
import json

host = '192.168.1.30:8800'
connection = http.client.HTTPSConnection('api.github.com')

headers = {'Content-type': 'application/json'}

cmd= {"id":1,"type":"temp","method":"get",index:1}
json_cmd = json.dumps(cmd)

connection.request('POST', '/api/v1', json_cmd, headers)

response = connection.getresponse()

print resp.status
print resp.reason
print resp.read()
print response.read().decode()
'''


import httplib, urllib
import sys
import json

host = '192.168.1.30'
port=8800

body = '{"id":1,"type":"temp","method":"get",index:1}'
headers = {"Content-type": "application/json","Accept": "*/*"}

conn = httplib.HTTPConnection(host,port)
conn.request("POST", "/api/v1", body, headers)

response = conn.getresponse()
print response.status,response.reason
data = response.read()
conn.close()

print "received from server:"+ data
#sys.stdout.flush()







##
## @file  chirpstack-downlink-multicast.py
##
## @brief Chirpstack multicast downlink helper
##
## The Clear BSD License
## Copyright Semtech Corporation 2024. All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted (subject to the limitations in the disclaimer
## below) provided that the following conditions are met:
##     * Redistributions of source code must retain the above copyright
##       notice, this list of conditions and the following disclaimer.
##     * Redistributions in binary form must reproduce the above copyright
##       notice, this list of conditions and the following disclaimer in the
##       documentation and/or other materials provided with the distribution.
##     * Neither the name of the Semtech corporation nor the
##       names of its contributors may be used to endorse or promote products
##       derived from this software without specific prior written permission.
##
## NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY
## THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
## CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
## NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
## PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SEMTECH CORPORATION BE
## LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
## CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
## SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
## INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
## CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
## ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
## POSSIBILITY OF SUCH DAMAGE.
##

 
import grpc
import os
import sys
from chirpstack_api import api

API_SERVER = "localhost" # Address of the Network Server
API_PORT = 8080          # API port of the Network Server
MULTICAST_GROUP_ID = "e84cd69a-2a05-4da0-8eb8-9122a43102ea"
FPORT= 10               # Fport of the downlink message
PAYLOAD = "010102020303040404"  # Payload of the downlink message

CHIRPSTACK_API_KEY = os.environ.get("CHIRPSTACK_API_KEY", None)
if CHIRPSTACK_API_KEY is None:
    sys.exit("CHIRPSTACK_API_KEY should be an environment variable")


channel = grpc.insecure_channel(API_SERVER+":"+str(API_PORT))
client = api.MulticastGroupServiceStub(channel)
auth_token = [("authorization", "Bearer %s" % CHIRPSTACK_API_KEY)]
mc_downlink = api.MulticastGroupQueueItem()
mc_downlink.multicast_group_id = MULTICAST_GROUP_ID
mc_downlink.f_port = FPORT
mc_downlink.data = bytes.fromhex(PAYLOAD)
request = api.EnqueueMulticastGroupQueueItemRequest()
request.queue_item.CopyFrom(mc_downlink)
resp = client.Enqueue(request, metadata=auth_token)
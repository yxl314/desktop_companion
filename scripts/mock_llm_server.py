#!/usr/bin/env python3
"""本地 OpenAI 兼容 mock 服务：离线验证流式对话链路。

启动：
    python scripts/mock_llm_server.py [端口，默认 8765]
然后在控制中心把 Base URL 设为 http://127.0.0.1:8765/v1，任意模型名即可。

仅实现两个接口：
    GET  /v1/models
    POST /v1/chat/completions   （stream=true / false 都支持）
"""

import json
import sys
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 8765


class Handler(BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"

    def log_message(self, fmt, *args):  # 安静一点
        pass

    def _send_json(self, status, payload):
        body = json.dumps(payload, ensure_ascii=False).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        if self.path.rstrip("/") == "/v1/models":
            self._send_json(200, {
                "object": "list",
                "data": [
                    {"id": "mock-chat", "object": "model", "owned_by": "mock"},
                ],
            })
        else:
            self._send_json(404, {"error": {"message": "not found"}})

    def do_POST(self):
        if self.path.rstrip("/") != "/v1/chat/completions":
            self._send_json(404, {"error": {"message": "not found"}})
            return

        length = int(self.headers.get("Content-Length", "0"))
        try:
            request = json.loads(self.rfile.read(length) or b"{}")
        except json.JSONDecodeError:
            self._send_json(400, {"error": {"message": "bad json"}})
            return

        user_text = ""
        for message in request.get("messages", []):
            if message.get("role") == "user":
                user_text = message.get("content", "")

        pieces = ["收到「", user_text.strip()[:20], "」啦，", "我是本地 mock 服务，", "流式链路正常。"]
        model = request.get("model", "mock-chat")

        if not request.get("stream"):
            self._send_json(200, {
                "id": "mock-1",
                "object": "chat.completion",
                "model": model,
                "choices": [{
                    "index": 0,
                    "finish_reason": "stop",
                    "message": {"role": "assistant", "content": "".join(pieces)},
                }],
            })
            return

        self.send_response(200)
        self.send_header("Content-Type", "text/event-stream")
        self.send_header("Cache-Control", "no-cache")
        self.send_header("Connection", "close")
        self.end_headers()

        for piece in pieces:
            chunk = {
                "id": "mock-1",
                "object": "chat.completion.chunk",
                "model": model,
                "choices": [{"index": 0, "finish_reason": None,
                             "delta": {"content": piece}}],
            }
            self.wfile.write(("data: " + json.dumps(chunk, ensure_ascii=False) + "\n\n").encode("utf-8"))
            self.wfile.flush()
            time.sleep(0.15)

        self.wfile.write(b"data: [DONE]\n\n")
        self.wfile.flush()
        self.close_connection = True


if __name__ == "__main__":
    print(f"mock llm server listening on http://127.0.0.1:{PORT}/v1")
    ThreadingHTTPServer(("127.0.0.1", PORT), Handler).serve_forever()

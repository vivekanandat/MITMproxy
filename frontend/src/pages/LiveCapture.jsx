import { useEffect, useState, useRef } from "react";
import { useParams , useNavigate } from "react-router-dom";
import { LineChart, Line, XAxis, YAxis, Tooltip, ResponsiveContainer } from "recharts";
import { getSocket } from "../lib/socket";




export default function LiveCapture() {
  const { id } = useParams();
  const [requests, setRequests] = useState([]);
  const [hostFilter, setHostFilter] = useState("");
  const [flaggedOnly, setFlaggedOnly] = useState(false);
  const [rateHistory, setRateHistory] = useState([]);
  const countRef = useRef(0);

  useEffect(() => {
    const socket = getSocket();
    socket.connect();
    socket.emit("watch_session", id);

    socket.on("new_request", (req) => {
      if (req.session_id !== id) return;
      setRequests((prev) => [req, ...prev].slice(0, 500));
      countRef.current += 1;
    });

    const tick = setInterval(() => {
      setRateHistory((prev) => {
        const next = [...prev, { time: new Date().toLocaleTimeString(), count: countRef.current }];
        countRef.current = 0;
        return next.slice(-30);
      });
    }, 1000);

    return () => {
      clearInterval(tick);
      socket.emit("unwatch_session", id);
      socket.off("new_request");
      socket.disconnect();
    };
  }, [id]);

  const filtered = requests.filter((r) => {
    if (hostFilter && !r.host.includes(hostFilter)) return false;
    if (flaggedOnly && !r.flagged) return false;
    return true;
  });
    const navigate = useNavigate();
  return (
    <div className="min-h-screen bg-gray-950 text-gray-100 p-8">
      <div className="max-w-5xl mx-auto space-y-6">
      <div className="flex items-center justify-between">
          <h1 className="text-2xl font-semibold">Live Capture</h1>
          <button
            onClick={() => navigate(`/sessions/${id}/end`)}
            className="px-4 py-2 rounded bg-red-600 hover:bg-red-500 text-sm font-medium"
          >
            End Session
          </button>
        </div>

        <div className="bg-gray-900 rounded-lg p-4 h-40">
          <ResponsiveContainer width="100%" height="100%">
            <LineChart data={rateHistory}>
              <XAxis dataKey="time" hide />
              <YAxis allowDecimals={false} stroke="#888" />
              <Tooltip />
              <Line type="monotone" dataKey="count" stroke="#3b82f6" dot={false} />
            </LineChart>
          </ResponsiveContainer>
        </div>

        <div className="flex gap-3">
          <input
            placeholder="Filter by host"
            value={hostFilter}
            onChange={(e) => setHostFilter(e.target.value)}
            className="px-3 py-2 rounded bg-gray-800 outline-none flex-1"
          />
          <label className="flex items-center gap-2 text-sm">
            <input type="checkbox" checked={flaggedOnly} onChange={(e) => setFlaggedOnly(e.target.checked)} />
            Flagged only
          </label>
        </div>

        <table className="w-full text-sm">
          <thead className="text-gray-400 text-left">
            <tr>
              <th className="py-2">Time</th>
              <th>Method</th>
              <th>Host</th>
              <th>Status</th>
              <th>Size</th>
              <th></th>
            </tr>
          </thead>
          <tbody>
            {filtered.map((r) => (
              <tr key={r.id} className={`border-t border-gray-800 ${r.flagged ? "bg-red-950/40" : ""}`}>
                <td className="py-2">{new Date(r.timestamp).toLocaleTimeString()}</td>
                <td>{r.method}</td>
                <td className="truncate max-w-xs">{r.host}</td>
                <td>{r.status_code}</td>
                <td>{r.request_size}b</td>
                <td>{r.flagged ? "⚠️" : ""}</td>
              </tr>
            ))}
          </tbody>
        </table>
      </div>
    </div>
  );
}


import { useCallback, useEffect, useState } from "react";
import { strataDB } from "./api/stratadb";
import "./index.css";

interface KVEntry {
  key: string;
  value: string;
}

interface DBStats {
  memtable_size: number;
  sstable_count: number;
  auto_flush_success: boolean;
}

interface Health {
  success: boolean;
  status?: string;
  engine?: string;
}

function normalizeEntries(data: any): KVEntry[] {
  const candidates = [
    data?.entries,
    data?.results,
    data?.records,
    data?.data?.entries,
    data?.data?.results,
    data?.data,
  ];

  const entries = candidates.find(Array.isArray);
  if (!entries) return [];

  return entries.map((entry: any) => ({
    key: String(entry?.key ?? ""),
    value: String(entry?.value ?? ""),
  }));
}

function App() {
  const [connected, setConnected] = useState(false);
  const [loading, setLoading] = useState(false);
  const [message, setMessage] = useState("");
  const [error, setError] = useState("");

  const [stats, setStats] = useState<DBStats | null>(null);

  const [key, setKey] = useState("");
  const [value, setValue] = useState("");
  const [result, setResult] = useState<KVEntry | null>(null);

  const [startKey, setStartKey] = useState("");
  const [endKey, setEndKey] = useState("");
  const [scanResults, setScanResults] = useState<KVEntry[]>([]);
  const [hasScanned, setHasScanned] = useState(false);

  // Backend health check
  const checkHealth = useCallback(async () => {
    try {
      const data: Health = await strataDB.health();
      setConnected(data.success === true);
    } catch {
      setConnected(false);
    }
  }, []);

  // Fetch database statistics
  const fetchStats = useCallback(async () => {
    try {
      const data = await strataDB.stats();

      setStats({
        memtable_size: Number(data.memtable_size ?? 0),
        sstable_count: Number(data.sstable_count ?? 0),
        auto_flush_success: data.auto_flush_success === true,
      });
    } catch {
      setStats(null);
    }
  }, []);

  // Refresh dashboard
  const refreshDashboard = useCallback(async () => {
    await Promise.all([checkHealth(), fetchStats()]);
  }, [checkHealth, fetchStats]);

  // Automatic refresh every 5 seconds
  useEffect(() => {
    void refreshDashboard();

    const interval = window.setInterval(
      () => void refreshDashboard(),
      5000
    );

    return () => window.clearInterval(interval);
  }, [refreshDashboard]);

  // Common action handler
  const runAction = async (action: () => Promise<void>) => {
    setLoading(true);
    setError("");
    setMessage("");

    try {
      await action();
    } catch (err) {
      setError(
        err instanceof Error
          ? err.message
          : "Something went wrong."
      );
    } finally {
      setLoading(false);
      void fetchStats();
    }
  };

  // PUT: Insert or update
  const handlePut = () =>
    runAction(async () => {
      const cleanKey = key.trim();

      if (!cleanKey) {
        throw new Error("Please enter a key.");
      }

      await strataDB.put(cleanKey, value);

      setResult({
        key: cleanKey,
        value,
      });

      setMessage("Record inserted or updated successfully.");
    });

  // GET: Retrieve
  const handleGet = () =>
    runAction(async () => {
      const cleanKey = key.trim();

      if (!cleanKey) {
        throw new Error("Please enter a key.");
      }

      const data = await strataDB.get(cleanKey);

      const returnedValue =
        data.value ?? data.data?.value ?? data.data;

      if (
        returnedValue === undefined ||
        returnedValue === null
      ) {
        throw new Error(
          "The backend did not return a value."
        );
      }

      setResult({
        key: cleanKey,
        value: String(returnedValue),
      });

      setMessage("Record retrieved successfully.");
    });

  // DELETE
  const handleDelete = () =>
    runAction(async () => {
      const cleanKey = key.trim();

      if (!cleanKey) {
        throw new Error("Please enter a key.");
      }

      await strataDB.delete(cleanKey);

      setResult(null);
      setMessage("Delete request completed successfully.");
    });

  // RANGE SCAN
  const handleScan = () =>
    runAction(async () => {
      const start = startKey.trim();
      const end = endKey.trim();

      if (!start || !end) {
        throw new Error(
          "Enter both the start key and end key."
        );
      }

      if (start > end) {
        throw new Error(
          "Start key must be less than or equal to end key."
        );
      }

      const data = await strataDB.scan(start, end);

      setScanResults(normalizeEntries(data));
      setHasScanned(true);

      setMessage("Range scan completed.");
    });

  return (
    <div className="app-shell">

      {/* Sidebar */}
      <aside className="sidebar">

        <div className="brand">
          <div className="brand-icon">S</div>

          <div>
            <h1>StrataDB</h1>
            <span>STORAGE ENGINE</span>
          </div>
        </div>

        <nav className="sidebar-nav">

          <p className="nav-label">WORKSPACE</p>

          <a href="#overview" className="nav-item active">
            <span>◈</span> Overview
          </a>

          <a href="#explorer" className="nav-item">
            <span>⌕</span> Key Explorer
          </a>

          <a href="#range-scan" className="nav-item">
            <span>⇄</span> Range Scan
          </a>

          <a href="#statistics" className="nav-item">
            <span>▥</span> Statistics
          </a>

        </nav>

        <div className="sidebar-bottom">

          <div className="engine-card">

            <span className="engine-card-title">
              ENGINE STATUS
            </span>

            <div className="engine-status">

              <span
                className={`status-dot ${
                  connected ? "online" : ""
                }`}
              />

              {connected
                ? "Engine Online"
                : "Engine Offline"}

            </div>

            <span className="engine-url">
              127.0.0.1:8080
            </span>

            <span className="version">
              STRATADB · C++17
            </span>

          </div>

        </div>

      </aside>

      {/* Main Content */}
      <main className="main-content">

        {/* Header */}
        <header className="topbar">

          <div>
            <span className="breadcrumb">
              WORKSPACE / OVERVIEW
            </span>

            <h2>Dashboard</h2>
          </div>

          <div className="topbar-right">

            <div
              className={`connection-pill ${
                connected ? "connected" : ""
              }`}
            >
              <span
                className={`status-dot ${
                  connected ? "online" : ""
                }`}
              />

              {connected
                ? "Backend Connected"
                : "Backend Disconnected"}
            </div>

            <button
              className="refresh-button"
              onClick={() => void refreshDashboard()}
              disabled={loading}
            >
              ↻ Refresh
            </button>

          </div>

        </header>

        {/* Welcome Section */}
        <section
          id="overview"
          className="welcome-section"
        >

          <div>

            <p className="eyebrow">
              YOUR DATABASE WORKSPACE
            </p>

            <h1>
              Welcome to <span>StrataDB.</span>
            </h1>

            <p>
              A persistent key-value storage engine built
              with C++. Manage records and monitor your
              database from one place.
            </p>

          </div>

          <div
            className="welcome-symbol"
            aria-hidden="true"
          >
            S
          </div>

        </section>

        {/* Notifications */}
        {(message || error) && (

          <div
            className={`notice ${
              error ? "notice-error" : "notice-success"
            }`}
          >

            <span>{error || message}</span>

            <button
              aria-label="Dismiss notification"
              onClick={() => {
                setMessage("");
                setError("");
              }}
            >
              ×
            </button>

          </div>

        )}

        {/* Statistics */}
        <section
          id="statistics"
          className="stats-grid"
        >

          <div className="stat-card">

            <span className="stat-icon green">
              ▤
            </span>

            <span className="stat-title">
              MEMTABLE ENTRIES
            </span>

            <strong>
              {stats ? stats.memtable_size : "—"}
            </strong>

            <span className="stat-subtitle">
              Records currently in memory
            </span>

          </div>

          <div className="stat-card">

            <span className="stat-icon blue">
              ▥
            </span>

            <span className="stat-title">
              SSTABLE COUNT
            </span>

            <strong>
              {stats ? stats.sstable_count : "—"}
            </strong>

            <span className="stat-subtitle">
              Sorted tables persisted on disk
            </span>

          </div>

          <div className="stat-card">

            <span className="stat-icon purple">
              ↻
            </span>

            <span className="stat-title">
              LAST AUTO FLUSH
            </span>

            <strong>
              {stats
                ? stats.auto_flush_success
                  ? "Success"
                  : "Failed"
                : "—"}
            </strong>

            <span className="stat-subtitle">
              Result of the last automatic flush
            </span>

          </div>

          <div className="stat-card">

            <span className="stat-icon orange">
              ⌁
            </span>

            <span className="stat-title">
              BACKEND STATUS
            </span>

            <strong>
              {connected ? "Online" : "Offline"}
            </strong>

            <span className="stat-subtitle">
              Local REST API connection
            </span>

          </div>

        </section>

        {/* Key Value Explorer */}
        <section
          id="explorer"
          className="panel"
        >

          <div className="panel-heading">

            <div>

              <span className="section-kicker">
                DATA OPERATIONS
              </span>

              <h3>Key-Value Explorer</h3>

              <p>
                Insert, retrieve, update or delete a record.
              </p>

            </div>

            <span className="tag">
              CRUD
            </span>

          </div>

          <div className="form-grid">

            <label>
              KEY

              <input
                type="text"
                placeholder="Enter a key"
                value={key}
                onChange={(event) =>
                  setKey(event.target.value)
                }
              />

            </label>

            <label>
              VALUE

              <input
                type="text"
                placeholder="Enter a value"
                value={value}
                onChange={(event) =>
                  setValue(event.target.value)
                }
              />

            </label>

          </div>

          <div className="action-row">

            <button
              className="primary-button"
              onClick={handlePut}
              disabled={loading}
            >
              {loading
                ? "Working..."
                : "＋ Insert / Update"}
            </button>

            <button
              className="secondary-button"
              onClick={handleGet}
              disabled={loading}
            >
              Retrieve
            </button>

            <button
              className="danger-button"
              onClick={handleDelete}
              disabled={loading}
            >
              Delete
            </button>

          </div>

          {result && (

            <div className="result-box">

              <div className="result-title">
                OPERATION RESULT
              </div>

              <div className="result-line">

                <span>KEY</span>

                <code>{result.key}</code>

              </div>

              <div className="result-line">

                <span>VALUE</span>

                <code className="result-value">
                  {result.value}
                </code>

              </div>

            </div>

          )}

        </section>

        {/* Range Scan */}
        <section
          id="range-scan"
          className="panel"
        >

          <div className="panel-heading">

            <div>

              <span className="section-kicker">
                QUERY OPERATIONS
              </span>

              <h3>Range Scan</h3>

              <p>
                Find records whose keys fall within
                the selected boundaries.
              </p>

            </div>

            <span className="tag">
              RANGE QUERY
            </span>

          </div>

          {/* Both Start and End Key Inputs */}
          <div className="scan-form">

            <label>
              START KEY

              <input
                type="text"
                placeholder="e.g. apple"
                value={startKey}
                onChange={(event) =>
                  setStartKey(event.target.value)
                }
              />

            </label>

            <label>
              END KEY

              <input
                type="text"
                placeholder="e.g. mango"
                value={endKey}
                onChange={(event) =>
                  setEndKey(event.target.value)
                }
              />

            </label>

            <button
              className="primary-button"
              onClick={handleScan}
              disabled={loading}
            >
              {loading
                ? "Scanning..."
                : "Run Scan →"}
            </button>

          </div>

          {/* Scan Results */}
          <div className="table-wrapper">

            <table>

              <thead>
                <tr>
                  <th>KEY</th>
                  <th>VALUE</th>
                </tr>
              </thead>

              <tbody>

                {scanResults.length > 0 ? (

                  scanResults.map((entry, index) => (

                    <tr key={`${entry.key}-${index}`}>

                      <td>
                        <code>{entry.key}</code>
                      </td>

                      <td>{entry.value}</td>

                    </tr>

                  ))

                ) : (

                  <tr>

                    <td
                      colSpan={2}
                      className="empty-state"
                    >
                      {hasScanned
                        ? "No records found in this range."
                        : "Enter both boundaries and run a range scan."}
                    </td>

                  </tr>

                )}

              </tbody>

            </table>

          </div>

          <div className="table-footer">

            {scanResults.length} record
            {scanResults.length === 1 ? "" : "s"} returned

          </div>

        </section>

        {/* Footer */}
        <footer className="footer">

          <span>
            STRATADB · C++ STORAGE ENGINE
          </span>

          <span>
            REACT + TYPESCRIPT DASHBOARD
          </span>

        </footer>

      </main>

    </div>
  );
}

export default App;

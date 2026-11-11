import { useCallback, useEffect, useMemo, useState } from "react";
import { strataDB } from "./api/stratadb";
import type { KVEntry } from "./api/stratadb";

const PAGE_SIZES = [25, 50, 100];

function valueType(value: string): "JSON" | "Text" {
  try {
    JSON.parse(value);
    return "JSON";
  } catch {
    return "Text";
  }
}

function DataExplorer() {
  const [records, setRecords] = useState<KVEntry[]>([]);
  const [search, setSearch] = useState("");
  const [page, setPage] = useState(1);
  const [pageSize, setPageSize] = useState(25);
  const [selected, setSelected] = useState<KVEntry | null>(null);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState("");
  const [copyMessage, setCopyMessage] = useState("");

  const loadRecords = useCallback(async () => {
    setLoading(true);
    setError("");

    try {
      const response = await strataDB.records();
      const sorted = [...response.results].sort((a, b) =>
        a.key.localeCompare(b.key)
      );
      setRecords(sorted);
    } catch (err) {
      setError(
        err instanceof Error
          ? err.message
          : "Unable to load database records."
      );
    } finally {
      setLoading(false);
    }
  }, []);

  useEffect(() => {
    void loadRecords();
  }, [loadRecords]);

  const filteredRecords = useMemo(() => {
    const query = search.trim().toLocaleLowerCase();
    if (!query) return records;
    return records.filter((record) =>
      record.key.toLocaleLowerCase().includes(query)
    );
  }, [records, search]);

  const pageCount = Math.max(1, Math.ceil(filteredRecords.length / pageSize));
  const currentPage = Math.min(page, pageCount);
  const visibleRecords = filteredRecords.slice(
    (currentPage - 1) * pageSize,
    currentPage * pageSize
  );

  useEffect(() => {
    setPage(1);
  }, [search, pageSize]);

  const copy = async (label: string, value: string) => {
    try {
      await navigator.clipboard.writeText(value);
      setCopyMessage(`${label} copied.`);
    } catch {
      setCopyMessage(`Unable to copy the ${label.toLocaleLowerCase()}.`);
    }
  };

  const formattedValue = selected
    ? (() => {
        try {
          return JSON.stringify(JSON.parse(selected.value), null, 2);
        } catch {
          return selected.value;
        }
      })()
    : "";

  return (
    <section id="explorer" className="panel explorer-panel">
      <div className="panel-heading">
        <div>
          <span className="section-kicker">DATA OPERATIONS</span>
          <h3>Data Explorer</h3>
          <p>Browse the latest visible records in key order.</p>
        </div>
        <span className="tag">{records.length} RECORDS</span>
      </div>

      <div className="explorer-toolbar">
        <label className="explorer-search">
          SEARCH KEYS
          <input
            type="search"
            placeholder="Filter by key..."
            value={search}
            onChange={(event) => setSearch(event.target.value)}
          />
        </label>
        <button
          className="secondary-button"
          onClick={() => void loadRecords()}
          disabled={loading}
        >
          {loading ? "Loading..." : "↻ Refresh"}
        </button>
      </div>

      <p className="explorer-caption">
        Showing {filteredRecords.length} of {records.length} records. Search
        filtering and pagination happen in this browser after one complete
        backend snapshot.
      </p>

      {error ? (
        <div className="explorer-state explorer-error">
          <strong>Could not load records.</strong>
          <span>{error}</span>
          <button className="primary-button" onClick={() => void loadRecords()}>
            Retry
          </button>
        </div>
      ) : loading ? (
        <div className="explorer-state">Loading records...</div>
      ) : records.length === 0 ? (
        <div className="explorer-state">
          <strong>No records stored yet.</strong>
          <span>Insert a key-value record to see it here.</span>
        </div>
      ) : filteredRecords.length === 0 ? (
        <div className="explorer-state">
          <strong>No matching records.</strong>
          <span>Try a different key search.</span>
        </div>
      ) : (
        <>
          <div className="table-wrapper explorer-table-wrapper">
            <table>
              <thead>
                <tr>
                  <th>KEY</th>
                  <th>VALUE</th>
                  <th>TYPE</th>
                  <th>ACTIONS</th>
                </tr>
              </thead>
              <tbody>
                {visibleRecords.map((record) => (
                  <tr
                    className="explorer-row"
                    key={record.key}
                    onClick={() => {
                      setSelected(record);
                      setCopyMessage("");
                    }}
                  >
                    <td><code>{record.key}</code></td>
                    <td className="value-preview">{record.value}</td>
                    <td><span className="type-badge">{valueType(record.value)}</span></td>
                    <td>
                      <button
                        className="table-action"
                        onClick={(event) => {
                          event.stopPropagation();
                          setSelected(record);
                          setCopyMessage("");
                        }}
                      >
                        View
                      </button>
                    </td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>

          <div className="explorer-pagination">
            <span>
              Page {currentPage} of {pageCount} · {filteredRecords.length} total
            </span>
            <label>
              Page size
              <select
                value={pageSize}
                onChange={(event) => setPageSize(Number(event.target.value))}
              >
                {PAGE_SIZES.map((size) => <option key={size} value={size}>{size}</option>)}
              </select>
            </label>
            <button
              className="secondary-button"
              onClick={() => setPage((value) => Math.max(1, value - 1))}
              disabled={currentPage === 1}
            >
              Previous
            </button>
            <button
              className="secondary-button"
              onClick={() => setPage((value) => Math.min(pageCount, value + 1))}
              disabled={currentPage === pageCount}
            >
              Next
            </button>
          </div>
        </>
      )}

      {selected && (
        <div className="record-detail" role="dialog" aria-modal="true">
          <div className="detail-header">
            <div>
              <span className="section-kicker">RECORD DETAILS</span>
              <h4>{selected.key}</h4>
            </div>
            <button
              className="detail-close"
              aria-label="Close record details"
              onClick={() => setSelected(null)}
            >
              ×
            </button>
          </div>
          <div className="detail-meta">
            <span>TYPE <strong>{valueType(selected.value)}</strong></span>
            <span>SIZE <strong>{new TextEncoder().encode(selected.value).length} bytes</strong></span>
          </div>
          <div className="detail-field">
            <span>FULL KEY</span>
            <code>{selected.key}</code>
            <button className="table-action" onClick={() => void copy("Key", selected.key)}>Copy key</button>
          </div>
          <div className="detail-field">
            <span>FULL VALUE</span>
            <pre>{formattedValue}</pre>
            <button className="table-action" onClick={() => void copy("Value", selected.value)}>Copy value</button>
          </div>
          {copyMessage && <p className="copy-message">{copyMessage}</p>}
        </div>
      )}
    </section>
  );
}

export default DataExplorer;

# Agent Memory SDK – Demos

## Public demo

- **`agent_memory_demo.py`** – Store and recall by prefix. Shows an agent storing key/value pairs under a namespace and querying by prefix. No embeddings, local only.

  ```bash
  pip install -e .
  set SYNRIX_LIB_PATH=<path to dir with libsynrix.dll>
  python -m synrix.examples.agent_memory_demo
  ```

Other scripts in this folder may exist locally for development and are not part of the public repo.

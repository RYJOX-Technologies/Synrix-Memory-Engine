"""
SYNRIX Feedback Submission

Submit feedback and telemetry data to help improve SYNRIX.
All submissions are optional and privacy-respecting.
"""

import json
import requests
from typing import Optional, Dict, Any
from .telemetry import get_telemetry, TelemetryCollector


# Feedback submission endpoint (update when you have a server)
FEEDBACK_ENDPOINT = "https://feedback.synrix.ai/submit"  # Placeholder
GITHUB_ISSUES_ENDPOINT = "https://github.com/your-org/synrix-python-sdk/issues/new"


def submit_feedback(
    feedback: str,
    email: Optional[str] = None,
    include_telemetry: bool = True,
    method: str = "github"
) -> Dict[str, Any]:
    """
    Submit feedback about SYNRIX.
    
    Args:
        feedback: User feedback text
        email: Optional email for follow-up
        include_telemetry: Whether to include hardware/performance data
        method: Submission method ("github", "api", "export")
    
    Returns:
        Dictionary with submission result
    """
    telemetry = get_telemetry()
    
    if method == "github":
        return _submit_via_github(feedback, email, include_telemetry, telemetry)
    elif method == "api":
        return _submit_via_api(feedback, email, include_telemetry, telemetry)
    elif method == "export":
        return _submit_via_export(feedback, email, include_telemetry, telemetry)
    else:
        raise ValueError(f"Unknown submission method: {method}")


def _submit_via_github(
    feedback: str,
    email: Optional[str],
    include_telemetry: bool,
    telemetry: Optional[TelemetryCollector]
) -> Dict[str, Any]:
    """Submit feedback via GitHub issue"""
    payload = {
        "feedback": feedback,
    }
    
    if email:
        payload["email"] = email
    
    if include_telemetry and telemetry:
        payload["telemetry"] = telemetry.get_telemetry_summary()
    elif include_telemetry:
        # Include hardware info even if telemetry disabled
        collector = TelemetryCollector(enabled=False)
        payload["hardware"] = collector.get_hardware_info()
    
    # Format as GitHub issue body
    issue_body = f"""## Feedback

{feedback}

"""
    
    if email:
        issue_body += f"**Contact:** {email}\n\n"
    
    if include_telemetry and "telemetry" in payload:
        issue_body += "## Hardware Information\n\n"
        issue_body += "```json\n"
        issue_body += json.dumps(payload["telemetry"]["hardware"], indent=2)
        issue_body += "\n```\n\n"
        
        if payload["telemetry"]["operations"]["total"] > 0:
            issue_body += "## Performance Metrics\n\n"
            issue_body += "```json\n"
            issue_body += json.dumps(payload["telemetry"]["operations"], indent=2)
            issue_body += "\n```\n"
    
    # Create GitHub issue URL with pre-filled body
    issue_url = f"{GITHUB_ISSUES_ENDPOINT}?body={requests.utils.quote(issue_body)}&title={requests.utils.quote('User Feedback')}"
    
    return {
        "method": "github",
        "status": "success",
        "message": "Please submit feedback via GitHub issue",
        "url": issue_url,
        "payload": payload,  # For manual submission
    }


def _submit_via_api(
    feedback: str,
    email: Optional[str],
    include_telemetry: bool,
    telemetry: Optional[TelemetryCollector]
) -> Dict[str, Any]:
    """Submit feedback via API endpoint"""
    payload = {
        "feedback": feedback,
    }
    
    if email:
        payload["email"] = email
    
    if include_telemetry and telemetry:
        payload["telemetry"] = telemetry.get_telemetry_summary()
    elif include_telemetry:
        collector = TelemetryCollector(enabled=False)
        payload["hardware"] = collector.get_hardware_info()
    
    try:
        response = requests.post(
            FEEDBACK_ENDPOINT,
            json=payload,
            timeout=10
        )
        response.raise_for_status()
        
        return {
            "method": "api",
            "status": "success",
            "message": "Feedback submitted successfully",
            "response": response.json(),
        }
    except Exception as e:
        return {
            "method": "api",
            "status": "error",
            "message": f"Failed to submit feedback: {e}",
            "payload": payload,  # For manual submission
        }


def _submit_via_export(
    feedback: str,
    email: Optional[str],
    include_telemetry: bool,
    telemetry: Optional[TelemetryCollector]
) -> Dict[str, Any]:
    """Export feedback to JSON file"""
    payload = {
        "feedback": feedback,
        "timestamp": telemetry.get_telemetry_summary()["timestamp"] if telemetry else None,
    }
    
    if email:
        payload["email"] = email
    
    if include_telemetry and telemetry:
        payload["telemetry"] = telemetry.get_telemetry_summary()
    elif include_telemetry:
        collector = TelemetryCollector(enabled=False)
        payload["hardware"] = collector.get_hardware_info()
    
    from datetime import datetime
    filename = f"synrix_feedback_{datetime.now().strftime('%Y%m%d_%H%M%S')}.json"
    
    with open(filename, "w") as f:
        json.dump(payload, f, indent=2)
    
    return {
        "method": "export",
        "status": "success",
        "message": f"Feedback exported to {filename}",
        "filename": filename,
        "payload": payload,
    }


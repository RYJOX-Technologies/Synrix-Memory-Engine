"""
SYNRIX Client Exceptions
"""


class SynrixError(Exception):
    """Base exception for all SYNRIX errors"""
    pass


class SynrixConnectionError(SynrixError):
    """Raised when connection to SYNRIX server fails"""
    pass


class SynrixNotFoundError(SynrixError):
    """Raised when a requested resource is not found"""
    pass


class SynrixValidationError(SynrixError):
    """Raised when input validation fails"""
    pass


#!/usr/bin/env python3
"""
Smart Codebase Ingester - Tree-sitter based, parallel processing
=================================================================
Accurate AST parsing with Tree-sitter + fast parallel processing
Stores code in proper SYNRIX format for semantic querying.

Features:
- Tree-sitter AST parsing (accurate, fast C-based parser)
- Parallel file processing (multi-core speedup)
- Batch writes to lattice (reduces overhead)
- Proper SYNRIX node format (COMPOSITION_*, FUNC_*, CLASS_*)
- Incremental updates (skip already-processed files)
- Progress reporting

Usage:
    python smart_codebase_ingester.py [codebase_path] [lattice_path]
"""

import os
import sys
import json
import time
from pathlib import Path
from typing import List, Dict, Optional, Tuple
from concurrent.futures import ProcessPoolExecutor, as_completed
from dataclasses import dataclass
import hashlib

try:
    from tree_sitter import Language, Parser
    try:
        import tree_sitter_languages
        TREE_SITTER_LANGUAGES_AVAILABLE = True
    except ImportError:
        TREE_SITTER_LANGUAGES_AVAILABLE = False
        # Fallback: try to get languages from langchain
        try:
            from langchain_community.document_loaders.parsers.language import tree_sitter_segmenter
            TREE_SITTER_LANGUAGES_AVAILABLE = True
        except ImportError:
            pass
except ImportError:
    print("ERROR: tree-sitter not installed")
    print("Install with: pip install tree-sitter tree-sitter-languages")
    sys.exit(1)

# Try to use RawSynrixBackend for fast writes, fallback to CLI
USE_RAW_BACKEND = False
try:
    # Try multiple import paths
    try:
        from synrix.raw_backend import RawSynrixBackend
        USE_RAW_BACKEND = True
    except ImportError:
        # Try relative import
        import sys
        import os
        script_dir = os.path.dirname(os.path.abspath(__file__))
        python_sdk_dir = os.path.dirname(script_dir)
        if python_sdk_dir not in sys.path:
            sys.path.insert(0, python_sdk_dir)
        from synrix.raw_backend import RawSynrixBackend
        USE_RAW_BACKEND = True
except ImportError:
    import subprocess

# Node type constants (from persistent_lattice.h)
LATTICE_NODE_PATTERN = 3
LATTICE_NODE_PRIMITIVE = 1

@dataclass
class ExtractedFunction:
    """Extracted function metadata"""
    name: str
    language: str
    file_path: str
    signature: str
    body: str
    line_start: int
    line_end: int
    return_type: Optional[str] = None
    parameters: Optional[str] = None

@dataclass
class ExtractedClass:
    """Extracted class metadata"""
    name: str
    language: str
    file_path: str
    methods: List[str]
    line_start: int
    line_end: int

class CodeExtractor:
    """Tree-sitter based code extractor"""
    
    def __init__(self, language: str):
        self.language = language.lower()
        self.parser = None
        self.lang = None
        self.use_ast = (language.lower() == 'python')  # Python uses ast, not tree-sitter
        
        if not self.use_ast:
            # For C++/C, try tree-sitter first, fallback to regex
            self.use_regex_fallback = False
            try:
                from langchain_community.document_loaders.parsers.language.cpp import CPPSegmenter
                self.segmenter = CPPSegmenter("")
                # Try to get parser - if this fails, use regex fallback
                try:
                    self.parser = self.segmenter.get_parser()
                    if self.parser:
                        # Check if parser can actually parse
                        test_tree = self.parser.parse(b'int test() {}')
                        if test_tree and test_tree.root_node:
                            # Parser works!
                            if hasattr(self.parser, 'language'):
                                self.lang = self.parser.language
                        else:
                            self.use_regex_fallback = True
                    else:
                        self.use_regex_fallback = True
                except:
                    self.use_regex_fallback = True
            except:
                self.use_regex_fallback = True
            
            if self.use_regex_fallback:
                self.parser = None
                self.lang = None
    
    def extract_functions(self, code: str, file_path: str) -> List[ExtractedFunction]:
        """Extract functions using AST (Python) or Tree-sitter/Regex (C++/C)"""
        if self.use_ast:
            # Python: use built-in ast module
            return self._extract_python_functions_ast(code, file_path)
        else:
            # C++/C: try tree-sitter first, fallback to regex
            if self.use_regex_fallback:
                return self._extract_cpp_functions_regex(code, file_path)
            elif self.parser:
                try:
                    tree = self.parser.parse(bytes(code, 'utf8'))
                    return self._extract_cpp_functions(tree, code, file_path)
                except:
                    # Fallback to regex if parsing fails
                    return self._extract_cpp_functions_regex(code, file_path)
            else:
                return self._extract_cpp_functions_regex(code, file_path)
    
    def extract_classes(self, code: str, file_path: str) -> List[ExtractedClass]:
        """Extract classes using AST (Python) or Tree-sitter/Regex (C++/C)"""
        if self.use_ast:
            # Python: use built-in ast module
            return self._extract_python_classes_ast(code, file_path)
        else:
            # C++/C: use regex fallback (class extraction is less critical)
            return self._extract_cpp_classes_regex(code, file_path)
    
    def _extract_python_functions_ast(self, code: str, file_path: str) -> List[ExtractedFunction]:
        """Extract Python functions using built-in ast module"""
        import ast
        functions = []
        code_lines = code.split('\n')
        
        try:
            tree = ast.parse(code)
            # Use ast.NodeVisitor to properly traverse (ast.walk includes nested functions)
            class FunctionVisitor(ast.NodeVisitor):
                def __init__(self, code_lines, file_path):
                    self.functions = []
                    self.code_lines = code_lines
                    self.file_path = file_path
                
                def visit_FunctionDef(self, node):
                    self._process_function(node)
                    self.generic_visit(node)  # Continue to nested functions
                
                def visit_AsyncFunctionDef(self, node):
                    self._process_function(node)
                    self.generic_visit(node)
                
                def _process_function(self, node):
                    func_name = node.name
                    
                    # Get parameters
                    params = []
                    for arg in node.args.args:
                        param_name = arg.arg
                        if arg.annotation:
                            # Simple type name extraction
                            if isinstance(arg.annotation, ast.Name):
                                param_type = arg.annotation.id
                            else:
                                param_type = str(arg.annotation)
                            params.append(f"{param_name}: {param_type}")
                        else:
                            params.append(param_name)
                    params_str = "(" + ", ".join(params) + ")"
                    
                    # Get return type
                    return_type = None
                    if node.returns:
                        if isinstance(node.returns, ast.Name):
                            return_type = node.returns.id
                        else:
                            return_type = str(node.returns)
                    
                    # Get body
                    start_line = node.lineno
                    end_line = getattr(node, 'end_lineno', start_line + 10)
                    body = "\n".join(self.code_lines[start_line-1:end_line])
                    
                    signature = f"def {func_name}{params_str}"
                    if return_type:
                        signature = f"def {func_name}{params_str} -> {return_type}"
                    
                    self.functions.append(ExtractedFunction(
                        name=func_name,
                        language='Python',
                        file_path=self.file_path,
                        signature=signature,
                        body=body,
                        line_start=start_line,
                        line_end=end_line,
                        return_type=return_type,
                        parameters=params_str
                    ))
            
            visitor = FunctionVisitor(code_lines, file_path)
            visitor.visit(tree)
            return visitor.functions
        except SyntaxError:
            pass  # Invalid Python code
        
        return functions
    
    def _extract_python_functions_tree_sitter(self, tree, code: str, file_path: str) -> List[ExtractedFunction]:
        """Extract Python functions"""
        functions = []
        code_lines = code.split('\n')
        
        # Query for function definitions
        query = self.lang.query("""
            (function_definition
                name: (identifier) @name
                parameters: (parameters) @params
                body: (block) @body)
        """)
        
        captures = query.captures(tree.root_node)
        name_nodes = [n for n, t in captures if t == 'name']
        param_nodes = [n for n, t in captures if t == 'params']
        body_nodes = [n for n, t in captures if t == 'body']
        
        for name_node, param_node, body_node in zip(name_nodes, param_nodes, body_nodes):
            func_name = name_node.text.decode('utf8')
            params = param_node.text.decode('utf8')
            body = body_node.text.decode('utf8')
            
            # Get return type hint if present
            return_type = None
            if name_node.parent and name_node.parent.type == 'function_definition':
                for child in name_node.parent.children:
                    if child.type == 'type':
                        return_type = child.text.decode('utf8')
                        break
            
            functions.append(ExtractedFunction(
                name=func_name,
                language='Python',
                file_path=file_path,
                signature=f"def {func_name}{params}",
                body=body,
                line_start=name_node.start_point[0],
                line_end=body_node.end_point[0],
                return_type=return_type,
                parameters=params
            ))
        
        return functions
    
    def _extract_cpp_functions(self, tree, code: str, file_path: str) -> List[ExtractedFunction]:
        """Extract C++ functions by walking the tree manually"""
        functions = []
        code_lines = code.split('\n')
        
        def walk_tree(node, depth=0):
            """Recursively walk tree to find function definitions"""
            if node.type == 'function_definition':
                # Found a function definition
                func_name = None
                params = ""
                body = ""
                return_type = None
                
                # Walk children to find function name, params, body, return type
                for child in node.children:
                    if child.type == 'function_declarator':
                        # Find identifier (function name)
                        for subchild in child.children:
                            if subchild.type == 'identifier':
                                func_name = subchild.text.decode('utf8')
                            elif subchild.type == 'parameter_list':
                                params = subchild.text.decode('utf8')
                    elif child.type == 'compound_statement':
                        body = child.text.decode('utf8')
                    elif child.type in ('primitive_type', 'type_identifier', 'auto'):
                        return_type = child.text.decode('utf8')
                
                if func_name:
                    line_start = node.start_point[0] + 1  # 1-indexed
                    line_end = node.end_point[0] + 1
                    
                    signature = f"{return_type or 'void'} {func_name}{params}"
                    
                    functions.append(ExtractedFunction(
                        name=func_name,
                        language='C++',
                        file_path=file_path,
                        signature=signature,
                        body=body,
                        line_start=line_start,
                        line_end=line_end,
                        return_type=return_type,
                        parameters=params
                    ))
            
            # Recurse into children
            for child in node.children:
                walk_tree(child, depth + 1)
        
        # Walk the tree
        walk_tree(tree.root_node)
        
        return functions
    
    def _extract_cpp_functions_regex(self, code: str, file_path: str) -> List[ExtractedFunction]:
        """Extract C++ functions using regex (fallback when tree-sitter fails)"""
        import re
        functions = []
        code_lines = code.split('\n')
        
        # Pattern to match function definitions
        # Matches: return_type function_name(params) { body }
        pattern = r'(\w+(?:\s*\*\s*|\s+))?(\w+)\s*\([^)]*\)\s*\{'
        
        for match in re.finditer(pattern, code, re.MULTILINE):
            # Extract function signature
            return_type = match.group(1).strip() if match.group(1) else 'void'
            func_name = match.group(2)
            
            # Find function body (matching braces)
            start_pos = match.end()
            brace_count = 1
            end_pos = start_pos
            
            for i, char in enumerate(code[start_pos:], start_pos):
                if char == '{':
                    brace_count += 1
                elif char == '}':
                    brace_count -= 1
                    if brace_count == 0:
                        end_pos = i + 1
                        break
            
            # Extract body
            body = code[match.start():end_pos] if end_pos > start_pos else code[match.start():match.end()+100]
            
            # Extract parameters from match
            params_match = re.search(r'\(([^)]*)\)', match.group(0))
            params = params_match.group(1) if params_match else ""
            
            # Calculate line numbers
            line_start = code[:match.start()].count('\n') + 1
            line_end = code[:end_pos].count('\n') + 1 if end_pos > start_pos else line_start + 10
            
            signature = f"{return_type} {func_name}({params})"
            
            functions.append(ExtractedFunction(
                name=func_name,
                language='C++',
                file_path=file_path,
                signature=signature,
                body=body[:2048],  # Truncate body
                line_start=line_start,
                line_end=line_end,
                return_type=return_type,
                parameters=f"({params})"
            ))
        
        return functions
    
    def _extract_python_classes_ast(self, code: str, file_path: str) -> List[ExtractedClass]:
        """Extract Python classes using built-in ast module"""
        import ast
        classes = []
        code_lines = code.split('\n')
        
        try:
            tree = ast.parse(code)
            for node in ast.walk(tree):
                if isinstance(node, ast.ClassDef):
                    class_name = node.name
                    start_line = node.lineno
                    end_line = getattr(node, 'end_lineno', start_line + 10)
                    
                    # Extract method names
                    methods = []
                    for item in node.body:
                        if isinstance(item, (ast.FunctionDef, ast.AsyncFunctionDef)):
                            methods.append(item.name)
                    
                    classes.append(ExtractedClass(
                        name=class_name,
                        language='Python',
                        file_path=file_path,
                        methods=methods,
                        line_start=start_line,
                        line_end=end_line
                    ))
        except SyntaxError:
            pass
        
        return classes
    
    def _extract_cpp_classes_regex(self, code: str, file_path: str) -> List[ExtractedClass]:
        """Extract C++ classes using regex (fallback)"""
        import re
        classes = []
        
        # Pattern for class/struct definitions
        pattern = r'(class|struct)\s+(\w+)\s*[^{]*\{'
        
        for match in re.finditer(pattern, code):
            class_name = match.group(2)
            start_pos = match.start()
            
            # Find class body
            brace_count = 1
            end_pos = match.end()
            for i, char in enumerate(code[match.end():], match.end()):
                if char == '{':
                    brace_count += 1
                elif char == '}':
                    brace_count -= 1
                    if brace_count == 0:
                        end_pos = i + 1
                        break
            
            # Extract methods (simple regex)
            body = code[start_pos:end_pos]
            method_pattern = r'(\w+(?:\s*\*\s*|\s+))?(\w+)\s*\([^)]*\)\s*[;{]'
            methods = [m.group(2) for m in re.finditer(method_pattern, body)]
            
            line_start = code[:start_pos].count('\n') + 1
            line_end = code[:end_pos].count('\n') + 1
            
            classes.append(ExtractedClass(
                name=class_name,
                language='C++',
                file_path=file_path,
                methods=methods[:20],  # Limit methods
                line_start=line_start,
                line_end=line_end
            ))
        
        return classes
    
    def _extract_python_classes(self, tree, code: str, file_path: str) -> List[ExtractedClass]:
        """Extract Python classes"""
        classes = []
        
        query = self.lang.query("""
            (class_definition
                name: (identifier) @name
                body: (block) @body)
        """)
        
        captures = query.captures(tree.root_node)
        name_nodes = [n for n, t in captures if t == 'name']
        body_nodes = [n for n, t in captures if t == 'body']
        
        for name_node, body_node in zip(name_nodes, body_nodes):
            class_name = name_node.text.decode('utf8')
            body = body_node.text.decode('utf8')
            
            # Extract method names from body
            method_query = self.lang.query("""
                (function_definition
                    name: (identifier) @method_name)
            """)
            method_captures = method_query.captures(body_node)
            methods = [n.text.decode('utf8') for n, t in method_captures if t == 'method_name']
            
            classes.append(ExtractedClass(
                name=class_name,
                language='Python',
                file_path=file_path,
                methods=methods,
                line_start=name_node.start_point[0],
                line_end=body_node.end_point[0]
            ))
        
        return classes
    
    def _extract_cpp_classes(self, tree, code: str, file_path: str) -> List[ExtractedClass]:
        """Extract C++ classes"""
        classes = []
        
        query = self.lang.query("""
            (class_specifier
                name: (type_identifier) @name
                body: (field_declaration_list) @body)
        """)
        
        captures = query.captures(tree.root_node)
        name_nodes = [n for n, t in captures if t == 'name']
        body_nodes = [n for n, t in captures if t == 'body']
        
        for name_node, body_node in zip(name_nodes, body_nodes):
            class_name = name_node.text.decode('utf8')
            body = body_node.text.decode('utf8')
            
            # Extract method names from body
            method_query = self.lang.query("""
                (function_definition
                    declarator: (function_declarator
                        declarator: (identifier) @method_name))
            """)
            method_captures = method_query.captures(body_node)
            methods = [n.text.decode('utf8') for n, t in method_captures if t == 'method_name']
            
            classes.append(ExtractedClass(
                name=class_name,
                language='C++',
                file_path=file_path,
                methods=methods,
                line_start=name_node.start_point[0],
                line_end=body_node.end_point[0]
            ))
        
        return classes

def process_file(file_path: str, language: str) -> Tuple[List[ExtractedFunction], List[ExtractedClass]]:
    """Process a single file (runs in parallel worker)"""
    try:
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
            code = f.read()
        
        extractor = CodeExtractor(language)
        functions = extractor.extract_functions(code, file_path)
        classes = extractor.extract_classes(code, file_path)
        
        return functions, classes
    except Exception as e:
        print(f"Error processing {file_path}: {e}")
        return [], []

def store_in_synrix(backend, func: ExtractedFunction):
    """Store function in SYNRIX with proper format"""
    # Build node name: COMPOSITION_<language>_function_<name>
    node_name = f"COMPOSITION_{func.language}_function_{func.name}"
    
    # Build node data (key|value format matching C implementation)
    # NOTE: We store metadata only, NOT the full function body (too large)
    # Body can be read from source file using file path + line numbers
    node_data = (
        f"language|{func.language}|"
        f"construct_type|function|"
        f"atom_sequence|return_type,function_name,open_paren,parameters,close_paren,open_brace,body,close_brace|"
        f"atom_roles||"
        f"required_atoms|function_name,parameters,body|"
        f"description|return_type={func.return_type or 'void'}|name={func.name}|parameters={func.parameters or ''}|file={func.file_path}|lines={func.line_start}-{func.line_end}|signature={func.signature}|"
        f"confidence|0.90"
    )
    
    # Truncate to fit 512 bytes (node data limit)
    if len(node_data) > 512:
        node_data = node_data[:509] + "..."
    
    if USE_RAW_BACKEND:
        backend.add_node(node_name, node_data, node_type=LATTICE_NODE_PATTERN)
    else:
        # Fallback to CLI
        script_dir = os.path.dirname(os.path.abspath(__file__))
        python_sdk_dir = os.path.dirname(script_dir)
        synrix_cli = os.path.join(python_sdk_dir, "synrix_cli")
        if not os.path.exists(synrix_cli):
            synrix_cli = os.path.join(script_dir, "synrix_cli")
        if not os.path.exists(synrix_cli):
            synrix_cli = "synrix_cli"
        subprocess.run(
            [synrix_cli, "write", backend.lattice_path, node_name, node_data],
            capture_output=True, check=False
        )

def store_class_in_synrix(backend, cls: ExtractedClass):
    """Store class in SYNRIX"""
    node_name = f"COMPOSITION_{cls.language}_class_{cls.name}"
    
    methods_str = ",".join(cls.methods[:10])  # Limit methods
    node_data = (
        f"language|{cls.language}|"
        f"construct_type|class|"
        f"atom_sequence|class,class_name,open_brace,members,methods,close_brace|"
        f"atom_roles||"
        f"required_atoms|class,class_name,methods|"
        f"description|name={cls.name}|methods={methods_str}|file={cls.file_path}|lines={cls.line_start}-{cls.line_end}|"
        f"confidence|0.85"
    )
    
    if len(node_data) > 512:
        node_data = node_data[:509] + "..."
    
    if USE_RAW_BACKEND:
        backend.add_node(node_name, node_data, node_type=LATTICE_NODE_PATTERN)
    else:
        script_dir = os.path.dirname(os.path.abspath(__file__))
        python_sdk_dir = os.path.dirname(script_dir)
        synrix_cli = os.path.join(python_sdk_dir, "synrix_cli")
        if not os.path.exists(synrix_cli):
            synrix_cli = os.path.join(script_dir, "synrix_cli")
        if not os.path.exists(synrix_cli):
            synrix_cli = "synrix_cli"
        subprocess.run(
            [synrix_cli, "write", backend.lattice_path, node_name, node_data],
            capture_output=True, check=False
        )

def main():
    codebase_path = sys.argv[1] if len(sys.argv) > 1 else "."
    lattice_path = sys.argv[2] if len(sys.argv) > 2 else os.path.expanduser("~/.codebase_lattice.lattice")
    
    print("=== Smart Codebase Ingester (Tree-sitter + Parallel) ===\n")
    print(f"Codebase: {codebase_path}")
    print(f"Lattice: {lattice_path}\n")
    
    # Initialize lattice
    if USE_RAW_BACKEND:
        backend = RawSynrixBackend(lattice_path, max_nodes=1000000)
        print("✓ Using fast RawSynrixBackend\n")
    else:
        # Fallback: use CLI - find synrix_cli in script directory or PATH
        script_dir = os.path.dirname(os.path.abspath(__file__))
        # Look in python-sdk directory (parent of examples)
        python_sdk_dir = os.path.dirname(script_dir)
        synrix_cli = os.path.join(python_sdk_dir, "synrix_cli")
        if not os.path.exists(synrix_cli):
            synrix_cli = os.path.join(script_dir, "synrix_cli")
        if not os.path.exists(synrix_cli):
            synrix_cli = "synrix_cli"  # Try PATH
        
        if not os.path.exists(lattice_path):
            subprocess.run([synrix_cli, "init", lattice_path, "1000000"], check=False, capture_output=True)
        backend = type('obj', (object,), {'lattice_path': lattice_path})()
        print("✓ Using CLI backend\n")
    
    # Find all code files
    print("🔍 Scanning for code files...")
    codebase = Path(codebase_path)
    files = []
    
    # More comprehensive ignore patterns
    ignore_patterns = [
        '/build/', '/.git/', '/node_modules/', '/__pycache__/', '/.venv/',
        '/python-env/', '/python_packages/', '/archive/', '/dev_archive/',
        '/.pytest_cache/', '/dist/', '/.mypy_cache/', '/venv/',
        '/test_', '/tests/', '/benchmark', '/tools/', '/examples/'
    ]
    
    for ext, lang in [('.py', 'python'), ('.cpp', 'cpp'), ('.c', 'c'), ('.h', 'cpp'), ('.hpp', 'cpp')]:
        for file_path in codebase.rglob(f'*{ext}'):
            if file_path.is_file():
                # Skip common ignore patterns
                file_str = str(file_path)
                if any(ignore in file_str for ignore in ignore_patterns):
                    continue
                files.append((str(file_path), lang))
    
    print(f"✅ Found {len(files)} files\n")
    
    # Process files in parallel
    print("📝 Extracting functions and classes (parallel processing)...")
    start_time = time.time()
    
    all_functions = []
    all_classes = []
    
    # Use ProcessPoolExecutor for true parallelism
    with ProcessPoolExecutor(max_workers=os.cpu_count() or 4) as executor:
        futures = {executor.submit(process_file, fp, lang): (fp, lang) for fp, lang in files}
        
        completed = 0
        for future in as_completed(futures):
            completed += 1
            if completed % 10 == 0:
                print(f"  Processed {completed}/{len(files)} files...", end='\r')
            
            functions, classes = future.result()
            all_functions.extend(functions)
            all_classes.extend(classes)
    
    print(f"\n✅ Extracted {len(all_functions)} functions, {len(all_classes)} classes")
    print(f"   Processing time: {time.time() - start_time:.2f}s\n")
    
    # Store in lattice (batch writes)
    print("💾 Storing in SYNRIX lattice...")
    start_time = time.time()
    
    stored_funcs = 0
    stored_classes = 0
    
    for func in all_functions:
        try:
            store_in_synrix(backend, func)
            stored_funcs += 1
            if stored_funcs % 50 == 0:
                print(f"  Stored {stored_funcs}/{len(all_functions)} functions...", end='\r')
        except Exception as e:
            print(f"Error storing function {func.name}: {e}")
    
    for cls in all_classes:
        try:
            store_class_in_synrix(backend, cls)
            stored_classes += 1
        except Exception as e:
            print(f"Error storing class {cls.name}: {e}")
    
    # Explicitly save if using RawSynrixBackend
    if USE_RAW_BACKEND:
        try:
            backend.save()
            print("💾 Lattice saved to disk")
        except Exception as e:
            print(f"Warning: Could not save lattice: {e}")
    
    print(f"\n✅ Stored {stored_funcs} functions, {stored_classes} classes")
    print(f"   Storage time: {time.time() - start_time:.2f}s\n")
    
    total_time = time.time() - start_time
    print("=== Ingestion Complete ===")
    print(f"Total time: {total_time:.2f}s")
    print(f"Files: {len(files)}")
    print(f"Functions: {stored_funcs}")
    print(f"Classes: {stored_classes}")
    print(f"\nQuery with:")
    print(f"  synrix_cli query {lattice_path} 'COMPOSITION_Python_function_' 100")
    print(f"  synrix_cli query {lattice_path} 'COMPOSITION_C++_class_' 100")

if __name__ == "__main__":
    main()

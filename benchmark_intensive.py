#!/usr/bin/env python3
"""
Intensive Performance Benchmark - Designed to stress-test optimizations
with larger datasets and more iterations to get measurable timing differences.
"""

import time
import statistics
import json
from datetime import datetime
from simpleparse.stt.TextTools import TextSearch

def create_test_data():
    """Create various test datasets for intensive benchmarking"""
    
    # Large ASCII text (1MB+)
    ascii_text_large = "The quick brown fox jumps over the lazy dog. " * 20000
    
    # Large Latin-1 text with accented characters
    latin1_text_large = "Café résumé naïve piñata señorita François Müller. " * 15000
    
    # Large mixed Unicode text
    mixed_unicode_large = "Hello 世界! Bonjour le monde! ¡Hola mundo! 🌍 " * 15000
    
    # Very large 2-byte Unicode text
    unicode2_large = "你好世界中文测试文本内容非常长。" * 10000
    
    # Large text for "not found" scenarios
    no_match_text = ("Lorem ipsum dolor sit amet consectetur adipiscing elit. " * 10000 +
                     "This text contains no target patterns anywhere in it. " * 5000)
    
    return {
        'ascii_large': ascii_text_large,
        'latin1_large': latin1_text_large,
        'mixed_unicode_large': mixed_unicode_large,
        'unicode2_large': unicode2_large,
        'no_match_text': no_match_text
    }

def benchmark_intensive_search(pattern, text, description, iterations=5000):
    """Benchmark a single search scenario intensively"""
    
    search_obj = TextSearch(pattern)
    
    # Warmup
    for _ in range(100):
        search_obj.search(text, 0, len(text))
    
    # Measure multiple runs
    times = []
    for _ in range(iterations):
        start = time.perf_counter()
        result = search_obj.search(text, 0, len(text))
        end = time.perf_counter()
        times.append(end - start)
    
    return {
        'description': description,
        'pattern': pattern,
        'text_length': len(text),
        'iterations': iterations,
        'mean_time': statistics.mean(times),
        'median_time': statistics.median(times),
        'min_time': min(times),
        'max_time': max(times),
        'std_dev': statistics.stdev(times) if len(times) > 1 else 0,
        'total_time': sum(times),
        'chars_per_second': len(text) / statistics.mean(times),
        'result': result[0] if result else -1
    }

def main():
    """Run intensive performance benchmark"""
    
    print("=" * 70)
    print("SimpleParse TextSearch INTENSIVE Performance Benchmark")
    print("=" * 70)
    print(f"Timestamp: {datetime.now().isoformat()}")
    
    test_data = create_test_data()
    results = []
    
    # Test scenarios designed to stress different optimization paths
    test_scenarios = [
        # 1-byte Unicode optimizations (ASCII)
        ("o", test_data['ascii_large'], "ASCII single char - 1-byte fast path"),
        ("fox", test_data['ascii_large'], "ASCII short pattern - 1-byte fast path"),
        ("jumps over the", test_data['ascii_large'], "ASCII long pattern - 1-byte fast path"),
        
        # 1-byte Unicode optimizations (Latin-1)
        ("é", test_data['latin1_large'], "Latin-1 single char - 1-byte fast path"),
        ("résumé", test_data['latin1_large'], "Latin-1 pattern - 1-byte fast path"),
        
        # Same-kind Unicode optimizations
        ("世", test_data['unicode2_large'], "2-byte Unicode single char - same-kind optimization"),
        ("世界", test_data['unicode2_large'], "2-byte Unicode pattern - same-kind optimization"),
        
        # Mixed-kind fallback (should be slower)
        ("世", test_data['mixed_unicode_large'], "2-byte char in mixed text - fallback"),
        ("🌍", test_data['mixed_unicode_large'], "4-byte emoji in mixed text - fallback"),
        
        # Not found scenarios (different optimization paths)
        ("NOTFOUND", test_data['no_match_text'], "ASCII not found - 1-byte optimization"),
        ("不存在", test_data['no_match_text'], "Unicode not found - fallback"),
        
        # Edge cases
        ("", test_data['ascii_large'], "Empty pattern - special case"),
        ("z" * 100, test_data['ascii_large'], "Very long pattern not found"),
    ]
    
    print()
    total_start = time.perf_counter()
    
    for i, (pattern, text, description) in enumerate(test_scenarios, 1):
        print(f"Running test {i:2d}/14: {description}")
        
        # Use fewer iterations for very large texts to keep benchmark reasonable
        iterations = 1000 if len(text) > 500000 else 3000
        
        result = benchmark_intensive_search(pattern, text, description, iterations)
        results.append(result)
        
        # Print immediate feedback
        mean_ms = result['mean_time'] * 1000
        throughput = result['chars_per_second'] / 1000000  # MB/s
        print(f"    Mean: {mean_ms:.3f}ms, Throughput: {throughput:.1f}MB/s, Found: {result['result']}")
    
    total_end = time.perf_counter()
    
    print("\n" + "=" * 70)
    print("INTENSIVE BENCHMARK RESULTS")
    print("=" * 70)
    
    # Group results by optimization type
    optimization_groups = {
        "1-byte fast path (ASCII)": [],
        "1-byte fast path (Latin-1)": [],
        "Same-kind optimization": [],
        "Fallback/Mixed": [],
        "Not found scenarios": [],
        "Special cases": []
    }
    
    for result in results:
        desc = result['description']
        if "ASCII" in desc and "1-byte fast path" in desc:
            optimization_groups["1-byte fast path (ASCII)"].append(result)
        elif "Latin-1" in desc and "1-byte fast path" in desc:
            optimization_groups["1-byte fast path (Latin-1)"].append(result)
        elif "same-kind optimization" in desc:
            optimization_groups["Same-kind optimization"].append(result)
        elif "not found" in desc:
            optimization_groups["Not found scenarios"].append(result)
        elif "Empty pattern" in desc or "Very long" in desc:
            optimization_groups["Special cases"].append(result)
        else:
            optimization_groups["Fallback/Mixed"].append(result)
    
    # Print grouped results
    for group_name, group_results in optimization_groups.items():
        if group_results:
            print(f"\n{group_name.upper()}:")
            print("-" * 50)
            
            for result in group_results:
                mean_ms = result['mean_time'] * 1000
                throughput_mb = result['chars_per_second'] / 1000000
                text_size_kb = result['text_length'] / 1024
                
                print(f"  {result['description']}:")
                print(f"    Time: {mean_ms:.3f}ms ±{result['std_dev']*1000:.3f}ms")
                print(f"    Throughput: {throughput_mb:.1f} MB/s")
                print(f"    Text size: {text_size_kb:.1f} KB")
                print(f"    Result: {'Found at ' + str(result['result']) if result['result'] >= 0 else 'Not found'}")
    
    # Summary statistics
    print(f"\nBENCHMARK SUMMARY:")
    print(f"Total benchmark time: {total_end - total_start:.2f} seconds")
    print(f"Fastest operation: {min(r['mean_time'] for r in results)*1000:.3f}ms")
    print(f"Slowest operation: {max(r['mean_time'] for r in results)*1000:.3f}ms")
    print(f"Average throughput: {statistics.mean(r['chars_per_second'] for r in results)/1000000:.1f} MB/s")
    
    # Save detailed results
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    filename = f"intensive_benchmark_{timestamp}.json"
    
    benchmark_data = {
        'metadata': {
            'timestamp': datetime.now().isoformat(),
            'total_time': total_end - total_start,
            'benchmark_type': 'intensive'
        },
        'results': results
    }
    
    with open(filename, 'w') as f:
        json.dump(benchmark_data, f, indent=2)
    
    print(f"\nDetailed results saved to: {filename}")

if __name__ == "__main__":
    main()
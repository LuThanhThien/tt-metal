from pathlib import Path 
from dev.py import *
from tt_metal.tools.profiler.common import *
from loguru import logger

def print_tracy_profile_log(output_result):
    output_path = Path(TT_METAL_HOME / output_result )
    if not output_path.exists():
        logger.error(f"Tracy profile log file not found: {output_path}")
        return

    tracy_profile_log_content = output_path.read_text(encoding="utf-8")
    tracy_profile_log_content = tracy_profile_log_content.replace("\n", "\n\n")
    
    logger.info(f"Tracy profile log content: {tracy_profile_log_content}")


if __name__ == "__main__":
    from optparse import OptionParser

    usage = "python print_tracy_profile_log.py [options]"
    parser = OptionParser(usage=usage)
    parser.allow_interspersed_args = False
    parser.add_option(
        "-o", "--output-result", action="store", help="Artifact output file", type="string", dest="output_result"
    )

    (options, args) = parser.parse_args()
    if not options.output_result:
        parser.error("Output result file is required")
    else:
        output_result = options.output_result
        print_tracy_profile_log(output_result)
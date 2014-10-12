./autogen.sh
./configure
make clean

# do not exit immediately if any command fails
set +e
     
# temp directory to store the scan-build report
SCAN_BUILD_TMPDIR=$( mktemp -d /tmp/scan-build.XXXXXX )

# directory to use for archiving the scan-build report
SCAN_BUILD_ARCHIVE="${WORKSPACE}/scan-build-archive"

# generate the scan-build report
scan-build -k -o ${SCAN_BUILD_TMPDIR} make
     
# get the directory name of the report created by scan-build
SCAN_BUILD_REPORT=$( find ${SCAN_BUILD_TMPDIR} -maxdepth 1 -not -empty -not -name `basename ${SCAN_BUILD_TMPDIR}` )
rc=$?

if [ -z "${SCAN_BUILD_REPORT}" ]; then
        echo ">>> No new bugs identified."
        echo ">>> No scan-build report has been generated"
else
        echo ">>> New scan-build report generated in ${SCAN_BUILD_REPORT}"
     
        if [ ! -d "${SCAN_BUILD_ARCHIVE}" ]; then
                echo ">>> Creating scan-build archive directory"
                install -d -o jenkins -g jenkins -m 0755 "${SCAN_BUILD_ARCHIVE}"
        else
                echo ">>> Removing any previous scan-build reports from ${SCAN_BUILD_ARCHIVE}"
                rm -f "${SCAN_BUILD_ARCHIVE}/*"
        fi
     
        echo ">>> Archiving scan-build report to ${SCAN_BUILD_ARCHIVE}"
        mv ${SCAN_BUILD_REPORT}/* "${SCAN_BUILD_ARCHIVE}/"
           
        echo ">>> Removing any temporary files and directories"
        rm -rf "${SCAN_BUILD_TMPDIR}"
fi

exit ${rc}

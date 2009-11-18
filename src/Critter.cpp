
#include "Critter.h"
#include "Debug.h"

#include "crucible/CrucibleConnector.h"
#include "crucible/Review.h"

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QTimer>

#include <iostream>

namespace po = boost::program_options;
using namespace std;

QMutex Debug::mutex;
bool Debug::enabled = false;

Critter::Critter(QObject *parent)
    : QObject(parent)
    , m_crucibleConnector(0)
{
}

Critter::Critter(CrucibleConnectorBase *connector, QObject *parent)
    : QObject(parent)
{
    m_crucibleConnector = new CrucibleConnector(connector);
}

void Critter::setOptions(po::variables_map vm) {
    m_vm = vm;
}

void Critter::exec() {
    QTimer::singleShot(0, this, SLOT(parseOptions()));
}

void Critter::parseOptions() {
    if (!m_crucibleConnector) {
        warning() << "No crucible connector!";
        qApp->quit();
        return;
    }

    const bool isCreateReview = m_vm.count("create");
    const bool isUpdateReview = m_vm.count("update");
    bool readFromStdIn = true;

    if (isCreateReview && isUpdateReview) {
        error() << "You can't create and update a review at the same time!";
        qApp->quit();
        return;
    }

    Review *review = new Review(this);

    if (isCreateReview) {
        QString author = m_crucibleConnector->user();
        if (m_vm.count("author")) {
            author = QString::fromStdString(m_vm["author"].as<string>());
        }
        review->setAuthor(author);
    }

    m_crucibleConnector->setReview(review);

    if (isUpdateReview) {
        const QString id = QString::fromStdString(m_vm["update"].as<string>());
        review->setId(id);
    }

    if (m_vm.count("title")) {
        const QString name = QString::fromStdString(m_vm["title"].as<string>());
        review->setName(name);
    }

    if (m_vm.count("objectives")) {
        const QString objectives = QString::fromStdString(m_vm["objectives"].as<string>());
        review->setDescription(objectives);
    }

    if (m_vm.count("project")) {
        const QString project = QString::fromStdString(m_vm["project"].as<string>());
        review->setProject(project);
    } else {
        review->setProject("CR");
    }

    if (m_vm.count("start")) {
        review->setShouldStart(true);
    }

    if (m_vm.count("creator")) {
        const QString creator = QString::fromStdString(m_vm["creator"].as<string>());
        review->setCreator(creator);
    }
    if (m_vm.count("moderator")) {
        const QString &moderator = QString::fromStdString(m_vm["moderator"].as<string>());
        review->setModerator(moderator);
    }

    if (m_vm.count("reviewers")) {
        const QVector<string> reviewers = QVector<string>::fromStdVector(m_vm["reviewers"].as< vector<string> >());
        foreach(string rev, reviewers) {
            review->addReviewer(QString::fromStdString(rev));
        }
    }
    
    if (m_vm.count("repository")) {
        const QString repo = QString::fromStdString(m_vm["repository"].as<string>());
        review->setRepository(repo);
    }

    if (m_vm.count("changeset")) {
        readFromStdIn = false;
        const QVector<string> changesets = QVector<string>::fromStdVector(m_vm["changeset"].as< vector<string> >());
        foreach(string cs, changesets) {
            review->addChangeset(QString::fromStdString(cs));
        }
    }

    if (m_vm.count("patch")) {
        readFromStdIn = false;
        const QString &patchFile = QString::fromStdString(m_vm["patch"].as<string>());
        QByteArray patchData = loadPatch(patchFile);
        if (!patchData.isEmpty()) {
            review->addPatch(patchData);
        }
    }

    if (readFromStdIn) {
        debug() << "Reading from stdin...";
        readStdIn(review);
    }

    if (isCreateReview) {
        m_crucibleConnector->createReview();
    } else if (isUpdateReview) {
        m_crucibleConnector->updateReview();
    }
    debug() << "All done, going to quit now...";
    qApp->quit();
}

void Critter::testConnection() {
    DEBUG_NOTIMPLEMENTED
//    m_crucibleConnector->testConnection();
}

QByteArray Critter::loadPatch(const QString &filename) const
{
    QFileInfo fi(QDir::current(), filename);

    debug() << "loading patch data from" << fi.filePath();

    QFile patch(fi.filePath());

    bool open = patch.open(QIODevice::ReadOnly);
    if (!open) {
        error() << "Could not open patch" << fi.filePath() << "Check file permissions";
        return QByteArray();
    }

    return patch.readAll();
}

void Critter::readStdIn(Review *review) {
    std::string input_line;

    QByteArray ba;

    bool isPatch = false;
    QString commitRevision = false;
    bool firstLine = true;

    while (!std::getline(std::cin, input_line).eof()) {
        const QString s = QString::fromStdString(input_line) + "\n";
        if (firstLine) {
            if (s.startsWith("diff") || s.startsWith("Index:")) {
                isPatch = true;
            }
            firstLine = false;
        }
        if (s.startsWith("Committed")) {
            commitRevision = QString(s);
            commitRevision.replace(QRegExp(".*(\\d+).*"), "\\1");
        }
        ba.append(s);
    }

    if (isPatch) {
        review->addPatch(ba);
    }

    if (!commitRevision.isEmpty()) {
        review->addChangeset(commitRevision);
    }
}

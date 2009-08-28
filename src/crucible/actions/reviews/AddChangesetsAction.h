#ifndef ADDCHANGESETSACTION_H
#define ADDCHANGESETSACTION_H

#include "AbstractReviewAction.h"

class QNetworkReply;

class AddChangesetsAction : public AbstractReviewAction
{
    Q_OBJECT

    public:
        AddChangesetsAction(Review *review, RestCommunicator *communicator, QObject *parent)
            : AbstractReviewAction(review, communicator, parent) {}
        virtual void run();

    signals:
        void changesetsAdded();

    protected:
        virtual void callFailed(QNetworkReply *reply);
        virtual void callSuccessful(QNetworkReply *reply);
};

#endif // ADDCHANGESETSACTION_H